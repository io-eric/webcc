#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <sys/stat.h>
#include <set>
#include <unistd.h>
#include <limits.h>
#include <cctype>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#if __has_include("webcc_schema.h")
#include "webcc_schema.h"
#define WEBCC_HAS_SCHEMA 1
#else
#define WEBCC_HAS_SCHEMA 0
#endif

// Small helpers used by the CLI to read and write files.

// Reads the entire contents of a file into a string.
static std::string read_file(const std::string &path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in)
        return std::string();
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Gets the full path to the currently running executable.
static std::string get_executable_path()
{
#ifdef __APPLE__
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        return std::string(path);
    }
    return "";
#else
    // Linux and WSL (Windows Subsystem for Linux)
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1)
    {
        return std::string(result, count);
    }
    return "";
#endif
}

// Gets the directory where the currently running executable is located.
static std::string get_executable_dir()
{
    std::string path = get_executable_path();
    size_t pos = path.find_last_of("/");
    if (pos != std::string::npos)
    {
        return path.substr(0, pos);
    }
    return ".";
}

// Writes a string to a file, creating parent directories if they don't exist.
static bool write_file(const std::string &path, const std::string &contents)
{
    // Ensure parent directories exist roughly (best-effort)
    size_t pos = path.find_last_of("/");
    if (pos != std::string::npos)
    {
        std::string dir = path.substr(0, pos);
        // try creating directory; ignore errors
        mkdir(dir.c_str(), 0755);
    }
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out)
        return false;
    out << contents;
    return out.good();
}

// -----------------------------------------------------------------------------
// 1. The Javascript Templates
// -----------------------------------------------------------------------------

// JS code to initialize the WebAssembly module and set up the environment.
const std::string JS_INIT_HEAD = R"(
const run = async () => {
    const response = await fetch('app.wasm');
    const bytes = await response.arrayBuffer();
    const mod = await WebAssembly.instantiate(bytes, {
        env: {
            // C++ calls this function to tell JS "I wrote commands, please execute them"
            webcc_js_flush: (ptr, size) => flush(ptr, size)
)";

// JS code to finalize WASM instantiation and get exported functions.
const std::string JS_INIT_TAIL = R"(
        }
    });

    // Get exports from WASM
    const { memory, main, __indirect_function_table: table } = mod.instance.exports;
)";

// JS code for the 'flush' function, which processes commands from C++.
const std::string JS_FLUSH_HEAD = R"(
    // Reusable text decoder to avoid garbage collection overhead
    const decoder = new TextDecoder();
    let view = new DataView(memory.buffer);
    let u8 = new Uint8Array(memory.buffer);
    let i32 = new Int32Array(memory.buffer);
    let f32 = new Float32Array(memory.buffer);
    const string_cache = [];

    function flush(ptr, size) {
        if (size === 0) return;

        if (view.buffer !== memory.buffer) {
            view = new DataView(memory.buffer);
            u8 = new Uint8Array(memory.buffer);
            i32 = new Int32Array(memory.buffer);
            f32 = new Float32Array(memory.buffer);
        }

        let pos = ptr;
        const end = ptr + size;
        string_cache.length = 0;

        // Loop through the buffer
        while (pos < end) {
            if (pos + 4 > end) {
                console.error("WebCC: Unexpected end of buffer reading opcode");
                break;
            }
            const opcode = i32[pos >> 2];
            pos += 4;

            switch (opcode) {
)";

// The constant "footer" for the generated JS file.
const std::string JS_TAIL = R"(
                default:
                    console.error("Unknown opcode:", opcode);
                    return;
            }
        }
    }

    // Run the C++ main function
    if (main) main();
};
run();
)";

// -----------------------------------------------------------------------------
// 2. The Feature Database
// -----------------------------------------------------------------------------

// The Feature Database is driven by `schema.def`.
// It defines the available commands and events that can be used to communicate
// between C++ and JavaScript.

// Represents a parameter for a command or event.
struct Param
{
    std::string type;
    std::string name; // optional; if empty we'll generate argN
};

// Represents a command definition from `schema.def`.
struct CommandDef
{
    std::string ns;   // Namespace
    std::string name; // NAME token
    uint8_t opcode;
    std::string func_name;     // C++ function name to search for
    std::vector<Param> params; // list of parameters
    std::string action;        // JS action body (using arg0.. or custom names)
    std::string return_type;   // Optional return type
};

// Represents an event definition from `schema.def`.
struct EventDef
{
    std::string ns;
    std::string name;
    uint8_t opcode;
    std::vector<Param> params;
};

// Holds all command and event definitions.
struct Defs
{
    std::vector<CommandDef> commands;
    std::vector<EventDef> events;
};

// Loads and parses the command and event definitions from a file (e.g., schema.def).
// The file format is a pipe-separated list of fields per line.
// For commands: NAMESPACE|command|NAME|FUNC_NAME|TYPES|ACTION
// For events:   NAMESPACE|event|NAME|ARGS
static Defs load_defs(const std::string &path)
{
    std::cout << "[WebCC] Loading definitions from " << path << std::endl;
    Defs out;
    std::string contents = read_file(path);
    if (contents.empty())
    {
        std::cerr << "[WebCC] Error: Definition file is empty or missing: " << path << std::endl;
        exit(1);
    }
    std::istringstream ss(contents);
    std::string line;
    uint8_t current_cmd_opcode = 1;
    uint8_t current_event_opcode = 1;
    int line_num = 0;

    while (std::getline(ss, line))
    {
        line_num++;
        // trim
        size_t p = line.find_first_not_of(" \t\r\n");
        if (p == std::string::npos)
            continue;
        if (line[p] == '#')
            continue;
        // split by '|'
        auto parts = std::vector<std::string>();
        size_t start = 0;
        // We expect at least 4 separators
        while (true)
        {
            size_t pos = line.find('|', start);
            if (pos == std::string::npos)
            {
                parts.push_back(line.substr(start));
                break;
            }
            parts.push_back(line.substr(start, pos - start));
            start = pos + 1;
        }

        if (parts.size() < 4)
        {
            std::cerr << "[WebCC] Warning: Skipping malformed line " << line_num << ": " << line << std::endl;
            continue;
        }

        std::string ns = parts[0];
        std::string kind = "command";
        int name_idx = 1;

        // Check if second column is explicit kind
        if (parts[1] == "event" || parts[1] == "command")
        {
            kind = parts[1];
            name_idx = 2;
        }

        if (kind == "event")
        {
            // NAMESPACE|event|NAME|ARGS
            if (parts.size() <= name_idx + 1)
                continue;
            EventDef e;
            e.ns = ns;
            e.name = parts[name_idx];
            e.opcode = current_event_opcode++;

            std::istringstream tss(parts[name_idx + 1]);
            std::string tkn;
            while (tss >> tkn)
            {
                Param p;
                size_t colon = tkn.find(':');
                if (colon == std::string::npos)
                {
                    p.type = tkn;
                    p.name = "";
                }
                else
                {
                    p.type = tkn.substr(0, colon);
                    p.name = tkn.substr(colon + 1);
                }
                e.params.push_back(p);
            }
            out.events.push_back(e);
        }
        else
        {
            // NAMESPACE|[command]|NAME|FUNC_NAME|TYPES|ACTION
            if (parts.size() <= name_idx + 3)
                continue;
            CommandDef c;
            c.ns = ns;
            c.name = parts[name_idx];
            c.opcode = current_cmd_opcode++;
            c.func_name = parts[name_idx + 1];
            // types
            std::istringstream tss(parts[name_idx + 2]);
            std::string tkn;
            while (tss >> tkn)
            {
                Param p;
                size_t colon = tkn.find(':');
                if (colon == std::string::npos)
                {
                    p.type = tkn;
                    p.name = "";
                }
                else
                {
                    p.type = tkn.substr(0, colon);
                    p.name = tkn.substr(colon + 1);
                }

                if (p.type == "RET")
                {
                    c.return_type = p.name;
                }
                else
                {
                    c.params.push_back(p);
                }
            }
            // Reconstruct action by finding the start position in the line
            size_t action_pos = 0;
            int pipes_needed = name_idx + 3;
            for (int i = 0; i < pipes_needed; ++i)
            {
                action_pos = line.find('|', action_pos) + 1;
            }
            c.action = line.substr(action_pos);
            out.commands.push_back(c);
        }
    }
    std::cout << "[WebCC] Loaded " << out.commands.size() << " commands and " << out.events.size() << " events." << std::endl;
    return out;
}

#if WEBCC_HAS_SCHEMA
static Defs load_defs_from_schema()
{
    std::cout << "[WebCC] Loading definitions from compiled-in schema..." << std::endl;
    Defs out;

    for (const auto *c = webcc::SCHEMA_COMMANDS; c->ns != nullptr; ++c)
    {
        CommandDef cmd;
        cmd.ns = c->ns;
        cmd.name = c->name;
        cmd.opcode = c->opcode;
        cmd.func_name = c->func_name;
        cmd.return_type = c->return_type;
        cmd.action = c->action;
        for (int i = 0; i < c->num_params; ++i)
        {
            Param p;
            p.type = c->params[i].type;
            p.name = c->params[i].name;
            cmd.params.push_back(p);
        }
        out.commands.push_back(cmd);
    }

    for (const auto *e = webcc::SCHEMA_EVENTS; e->ns != nullptr; ++e)
    {
        EventDef evt;
        evt.ns = e->ns;
        evt.name = e->name;
        evt.opcode = e->opcode;
        for (int i = 0; i < e->num_params; ++i)
        {
            Param p;
            p.type = e->params[i].type;
            p.name = e->params[i].name;
            evt.params.push_back(p);
        }
        out.events.push_back(evt);
    }

    std::cout << "[WebCC] Loaded " << out.commands.size() << " commands and " << out.events.size() << " events." << std::endl;
    return out;
}
#endif

// Generates the JavaScript 'case' block for a single command's opcode.
static std::string gen_js_case(const CommandDef &c)
{
    std::stringstream ss;
    ss << "            case " << (int)c.opcode << ": {\n";
    // Declare typed variables using the parameter names from the def file
    for (size_t i = 0; i < c.params.size(); ++i)
    {
        const auto &p = c.params[i];
        std::string varName = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
        if (p.type == "uint8" || p.type == "uint32")
        {
            ss << "                if (pos + 4 > end) { console.error('WebCC: OOB " << varName << "'); break; }\n";
            ss << "                const " << varName << " = i32[pos >> 2]; pos += 4;\n";
        }
        else if (p.type == "int32")
        {
            ss << "                if (pos + 4 > end) { console.error('WebCC: OOB " << varName << "'); break; }\n";
            ss << "                const " << varName << " = i32[pos >> 2]; pos += 4;\n";
        }
        else if (p.type == "float32")
        {
            ss << "                if (pos + 4 > end) { console.error('WebCC: OOB " << varName << "'); break; }\n";
            ss << "                const " << varName << " = f32[pos >> 2]; pos += 4;\n";
        }
        else if (p.type == "func_ptr")
        {
            ss << "                if (pos + 4 > end) { console.error('WebCC: OOB " << varName << "'); break; }\n";
            ss << "                const " << varName << " = i32[pos >> 2]; pos += 4;\n";
        }
        else if (p.type == "string")
        {
            ss << "                if (pos + 4 > end) { console.error('WebCC: OOB " << varName << "_tag'); break; }\n";
            ss << "                const " << varName << "_tag = i32[pos >> 2]; pos += 4;\n";
            ss << "                let " << varName << ";\n";
            ss << "                if (" << varName << "_tag === 0) {\n";
            ss << "                    if (pos + 4 > end) { console.error('WebCC: OOB " << varName << "_id'); break; }\n";
            ss << "                    const " << varName << "_id = i32[pos >> 2]; pos += 4;\n";
            ss << "                    " << varName << " = string_cache[" << varName << "_id];\n";
            ss << "                } else {\n";
            ss << "                    if (pos + 4 > end) { console.error('WebCC: OOB " << varName << "_len'); break; }\n";
            ss << "                    const " << varName << "_len = i32[pos >> 2]; pos += 4;\n";
            ss << "                    const " << varName << "_padded = (" << varName << "_len + 3) & ~3;\n";
            ss << "                    if (pos + " << varName << "_padded > end) { console.error('WebCC: OOB " << varName << "_data'); break; }\n";
            ss << "                    " << varName << " = decoder.decode(u8.subarray(pos, pos + " << varName << "_len)); pos += " << varName << "_padded;\n";
            ss << "                    string_cache.push(" << varName << ");\n";
            ss << "                }\n";
        }
        else
        {
            ss << "                // Unknown type: " << p.type << "\n";
        }
    }
    ss << "                " << c.action << "\n";
    ss << "                break;\n            }\n";
    return ss.str();
}

// Generates the `webcc_schema.h` header file.
// This file contains a C++ representation of the command schema, which can be
// used for introspection or other tooling.
static void emit_schema_header(const Defs &defs)
{
    std::stringstream out;
    out << "// GENERATED FILE - DO NOT EDIT\n";
    out << "#pragma once\n";
    out << "#include <cstdint>\n\n";
    out << "namespace webcc {\n\n";

    out << "struct SchemaParam {\n";
    out << "    const char* type;\n";
    out << "    const char* name;\n";
    out << "};\n\n";

    out << "struct SchemaCommand {\n";
    out << "    const char* ns;\n";
    out << "    const char* name;\n";
    out << "    uint8_t opcode;\n";
    out << "    const char* func_name;\n";
    out << "    const char* return_type;\n";
    out << "    const char* action;\n";
    out << "    int num_params;\n";
    out << "    SchemaParam params[8];\n";
    out << "};\n\n";

    out << "struct SchemaEvent {\n";
    out << "    const char* ns;\n";
    out << "    const char* name;\n";
    out << "    uint8_t opcode;\n";
    out << "    int num_params;\n";
    out << "    SchemaParam params[8];\n";
    out << "};\n\n";

    out << "static const SchemaCommand SCHEMA_COMMANDS[] = {\n";
    for (const auto &d : defs.commands)
    {
        out << "    { \"" << d.ns << "\", \"" << d.name << "\", " << (int)d.opcode << ", ";
        out << "\"" << d.func_name << "\", \"" << d.return_type << "\", ";
        out << "R\"JS_ACTION(" << d.action << ")JS_ACTION\", ";
        out << d.params.size() << ", {";
        for (size_t i = 0; i < d.params.size(); ++i)
        {
            if (i > 0)
                out << ", ";
            std::string name = d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name;
            out << "{ \"" << d.params[i].type << "\", \"" << name << "\" }";
        }
        out << "} },\n";
    }
    out << "    { nullptr, nullptr, 0, nullptr, nullptr, nullptr, 0, {} }\n";
    out << "};\n\n";

    out << "static const SchemaEvent SCHEMA_EVENTS[] = {\n";
    for (const auto &d : defs.events)
    {
        out << "    { \"" << d.ns << "\", \"" << d.name << "\", " << (int)d.opcode << ", ";
        out << d.params.size() << ", {";
        for (size_t i = 0; i < d.params.size(); ++i)
        {
            if (i > 0)
                out << ", ";
            std::string name = d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name;
            out << "{ \"" << d.params[i].type << "\", \"" << name << "\" }";
        }
        out << "} },\n";
    }
    out << "    { nullptr, nullptr, 0, 0, {} }\n";
    out << "};\n\n";

    out << "} // namespace webcc\n";

    write_file("src/cli/webcc_schema.h", out.str());
    std::cout << "[WebCC] Emitted src/cli/webcc_schema.h" << std::endl;
}

// Generates the C++ header files for each namespace (e.g., webcc/dom.h).
// These headers provide the C++ functions that send commands to JavaScript.
static void emit_headers(const Defs &defs)
{
    std::cout << "[WebCC] Emitting headers..." << std::endl;
    // Emit per-namespace headers
    std::set<std::string> namespaces;
    for (const auto &d : defs.commands)
        namespaces.insert(d.ns);
    for (const auto &d : defs.events)
        namespaces.insert(d.ns);

    std::cout << "[WebCC] Found namespaces: ";
    for (const auto &ns : namespaces)
        std::cout << ns << " ";
    std::cout << std::endl;

    for (const auto &ns : namespaces)
    {
        std::stringstream out;
        out << "// GENERATED FILE - DO NOT EDIT\n";
        out << "#pragma once\n";
        out << "#include \"webcc.h\"\n\n";
        out << "namespace webcc::" << ns << " {\n";

        // Commands
        out << "    enum OpCode {\n";
        bool first = true;
        for (const auto &d : defs.commands)
        {
            if (d.ns != ns)
                continue;
            if (!first)
                out << ",\n";
            out << "        OP_" << d.name << " = 0x" << std::hex << (int)d.opcode << std::dec;
            first = false;
        }
        out << "\n    };\n\n";

        // Events
        bool has_events = false;
        for (const auto &d : defs.events)
            if (d.ns == ns)
                has_events = true;

        if (has_events)
        {
            out << "    enum EventType {\n";
            first = true;
            for (const auto &d : defs.events)
            {
                if (d.ns != ns)
                    continue;
                if (!first)
                    out << ",\n";
                out << "        EVENT_" << d.name << " = 0x" << std::hex << (int)d.opcode << std::dec;
                first = false;
            }
            out << "\n    };\n\n";

            out << "    enum EventMask {\n";
            int shift = 0;
            first = true;
            for (const auto &d : defs.events)
            {
                if (d.ns != ns)
                    continue;
                if (!first)
                    out << ",\n";
                out << "        MASK_" << d.name << " = 1 << " << shift++;
                first = false;
            }
            out << "\n    };\n\n";

            // Generate Event Structs
            for (const auto &d : defs.events)
            {
                if (d.ns != ns)
                    continue;

                std::string struct_name;
                bool next_upper = true;
                for(char c : d.name) {
                    if(c == '_') {
                        next_upper = true;
                    } else {
                        if(next_upper) {
                            struct_name += toupper(c);
                            next_upper = false;
                        } else {
                            struct_name += tolower(c);
                        }
                    }
                }
                struct_name += "Event";

                out << "    struct " << struct_name << " {\n";
                for (const auto &p : d.params)
                {
                    std::string type = p.type;
                    if (type == "int32") type = "int32_t";
                    else if (type == "uint32") type = "uint32_t";
                    else if (type == "float32") type = "float";
                    else if (type == "uint8") type = "uint8_t";
                    else if (type == "string") type = "const char*";
                    
                    std::string name = p.name;
                    out << "        " << type << " " << name << ";\n";
                }

                out << "\n        static " << struct_name << " parse(const uint8_t* data, uint32_t len) {\n";
                out << "            " << struct_name << " res;\n";
                out << "            uint32_t offset = 0;\n";
                for (const auto &p : d.params) {
                    if (p.type == "int32") {
                        out << "            res." << p.name << " = *(int32_t*)(data + offset); offset += 4;\n";
                    } else if (p.type == "uint32") {
                        out << "            res." << p.name << " = *(uint32_t*)(data + offset); offset += 4;\n";
                    } else if (p.type == "float32") {
                        out << "            res." << p.name << " = *(float*)(data + offset); offset += 4;\n";
                    } else if (p.type == "uint8") {
                        out << "            res." << p.name << " = *(uint8_t*)(data + offset); offset += 1;\n";
                    } else if (p.type == "string") {
                        out << "            uint16_t " << p.name << "_len = *(uint16_t*)(data + offset); offset += 2;\n";
                        out << "            res." << p.name << " = (const char*)(data + offset);\n";
                        out << "            offset += " << p.name << "_len;\n";
                    }
                }
                out << "            return res;\n";
                out << "        }\n";

                out << "    };\n\n";
            }
        }

        // Functions
        for (const auto &d : defs.commands)
        {
            if (d.ns != ns)
                continue;

            if (!d.return_type.empty())
            {
                std::string ret_type = d.return_type;
                if (ret_type == "int32")
                    ret_type = "int32_t";
                else if (ret_type == "uint32")
                    ret_type = "uint32_t";
                else if (ret_type == "float32")
                    ret_type = "float";

                // Generate extern "C" import
                out << "    extern \"C\" " << ret_type << " webcc_" << d.ns << "_" << d.func_name << "(";
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (i)
                        out << ", ";
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    if (p.type == "string")
                        out << "const char* " << name << ", uint32_t " << name << "_len";
                    else if (p.type == "float32")
                        out << "float " << name;
                    else if (p.type == "uint8")
                        out << "uint8_t " << name;
                    else if (p.type == "uint32")
                        out << "uint32_t " << name;
                    else if (p.type == "int32")
                        out << "int32_t " << name;
                    else
                        out << "/*unknown*/ void* " << name;
                }
                out << ");\n";

                // Generate inline wrapper
                out << "    inline " << ret_type << " " << d.func_name << "(";
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (i)
                        out << ", ";
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    if (p.type == "string")
                        out << "const char* " << name;
                    else if (p.type == "float32")
                        out << "float " << name;
                    else if (p.type == "uint8")
                        out << "uint8_t " << name;
                    else if (p.type == "uint32")
                        out << "uint32_t " << name;
                    else if (p.type == "int32")
                        out << "int32_t " << name;
                    else
                        out << "/*unknown*/ void* " << name;
                }
                out << "){\n";
                out << "        ::webcc::flush();\n";
                out << "        return webcc_" << d.ns << "_" << d.func_name << "(";
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (i)
                        out << ", ";
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    out << name;
                    if (p.type == "string")
                        out << ", webcc::strlen(" << name << ")";
                }
                out << ");\n";
                out << "    }\n\n";
                continue;
            }

            out << "    inline void " << d.func_name << "(";
            // param list
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                if (i)
                    out << ", ";
                const auto &p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                if (p.type == "uint8")
                    out << "uint8_t " << name;
                else if (p.type == "uint32")
                    out << "uint32_t " << name;
                else if (p.type == "int32")
                    out << "int32_t " << name;
                else if (p.type == "float32")
                    out << "float " << name;
                else if (p.type == "string")
                    out << "const char* " << name;
                else if (p.type == "func_ptr")
                    out << "void (*" << name << ")(float)";
                else
                    out << "/*unknown*/ void* " << name;
            }
            out << "){\n";
            out << "        push_command((uint32_t)OP_" << d.name << ");\n";
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                const auto &p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                if (p.type == "uint8")
                    out << "        push_data<uint32_t>((uint32_t)" << name << ");\n";
                else if (p.type == "uint32")
                    out << "        push_data<uint32_t>(" << name << ");\n";
                else if (p.type == "int32")
                    out << "        push_data<int32_t>(" << name << ");\n";
                else if (p.type == "float32")
                    out << "        push_data<float>(" << name << ");\n";
                else if (p.type == "string")
                    out << "        webcc::CommandBuffer::push_string(" << name << ", strlen(" << name << "));\n";
                else if (p.type == "func_ptr")
                    out << "        push_data<uint32_t>((uint32_t)(uintptr_t)" << name << ");\n";
                else
                    out << "        // unknown type: " << p.type << "\n";
            }
            out << "    }\n\n";
        }
        out << "} // namespace webcc::" << ns << "\n\n";

        write_file("include/webcc/" + ns + ".h", out.str());
        std::cout << "[WebCC] Emitted include/webcc/" << ns << ".h" << std::endl;
    }
    emit_schema_header(defs);
}

// Helper to check if 'text' contains 'word' as a whole identifier.
// This is used to avoid partial matches (e.g., 'draw' in 'drawString').
static bool contains_whole_word(const std::string &text, const std::string &word)
{
    size_t pos = 0;
    while ((pos = text.find(word, pos)) != std::string::npos)
    {
        // Check character before
        bool boundary_start = (pos == 0);
        if (!boundary_start)
        {
            char c = text[pos - 1];
            if (isalnum(c) || c == '_')
                boundary_start = false;
            else
                boundary_start = true;
        }

        // Check character after
        bool boundary_end = (pos + word.length() == text.length());
        if (!boundary_end)
        {
            char c = text[pos + word.length()];
            if (isalnum(c) || c == '_')
                boundary_end = false;
            else
                boundary_end = true;
        }

        if (boundary_start && boundary_end)
            return true;

        pos += 1;
    }
    return false;
}

// Scans a JS action string to find which resource maps it uses (e.g., elements, canvases).
static std::set<std::string> get_maps_from_action(const std::string &action)
{
    std::set<std::string> maps;
    std::vector<std::string> possible_maps = {"elements", "contexts", "audios", "websockets", "images", "webgl_shaders", "webgl_programs", "webgl_buffers", "textures", "webgl_uniforms",
                                              "webgpu_adapters", "webgpu_devices", "webgpu_queues", "webgpu_shaders", "webgpu_encoders", "webgpu_views", "webgpu_passes", "webgpu_buffers", "webgpu_pipelines"};
    for (const auto &map : possible_maps)
    {
        if (contains_whole_word(action, map))
        {
            maps.insert(map);
        }
    }
    return maps;
}

int main(int argc, char **argv)
{
    std::string defs_path = "schema.def";
    std::vector<std::string> input_files;
    bool generate_headers = false;
    std::string out_dir = ".";

    // Parse command-line arguments.
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--headers")
        {
            generate_headers = true;
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                defs_path = argv[++i];
            }
        }
        else if (arg == "--out")
        {
            if (i + 1 < argc)
            {
                out_dir = argv[++i];
            }
        }
        else
        {
            input_files.push_back(arg);
        }
    }

    // Ensure output directory exists
    if (out_dir != ".")
    {
        mkdir(out_dir.c_str(), 0755);
    }

    std::string build_dir = out_dir + "/.webcc_cache";

    // Load the command and event definitions.
    Defs defs;

    // If 'headers' command is given, generate headers and exit.
    if (generate_headers)
    {
        defs = load_defs(defs_path);
        emit_headers(defs);
        return 0;
    }

#if WEBCC_HAS_SCHEMA
    defs = load_defs_from_schema();
#else
    defs = load_defs(defs_path);
#endif

    if (input_files.empty())
    {
        std::cerr << "Usage: webcc [--defs <path>] [--out <dir>] <source.cc> ... or webcc headers" << std::endl;
        return 1;
    }

    std::string user_code;
    std::string source_files;

    // A. READ USER CODE (All files)
    // Concatenate all input source files into a single string for analysis.
    for (const auto &path : input_files)
    {
        std::string content = read_file(path);
        if (content.empty())
        {
            std::cerr << "Error: Could not read " << path << std::endl;
            return 1;
        }
        user_code += content + "\n";
        source_files += path + " ";
    }

    // B. GENERATE JS RUNTIME
    std::stringstream js_builder;

    std::cout << "[WebCC] Scanning source files for features..." << std::endl;
    // Load command defs and emit C++ helpers so `webcc::canvas::*` functions
    // are kept in sync with the command definitions.
    // auto defs = load_defs("schema.def"); // Already loaded
    // emit_headers(defs); // Not needed during build, headers should be pre-generated or included

    std::set<std::string> used_namespaces;
    std::set<std::string> used_maps;
    std::stringstream cases_builder;
    std::vector<std::string> generated_js_imports;

    // Analyze user code to find which webcc commands are used.
    for (const auto &d : defs.commands)
    {
        bool used = false;
        // 1. Check for qualified usage: ns::func or webcc::ns::func
        if (contains_whole_word(user_code, d.ns + "::" + d.func_name))
            used = true;
        else if (contains_whole_word(user_code, "webcc::" + d.ns + "::" + d.func_name))
            used = true;

        // 2. Check for using namespace
        if (!used)
        {
            bool ns_used = contains_whole_word(user_code, "using namespace webcc::" + d.ns) ||
                           contains_whole_word(user_code, "using namespace webcc");
            if (ns_used && contains_whole_word(user_code, d.func_name))
                used = true;
        }

        if (used)
        {
            std::cout << "  -> Found " << d.func_name << " (" << d.ns << "), embedding JS support." << std::endl;

            // Handle commands that have a return value. These are implemented as JS imports.
            if (!d.return_type.empty())
            {
                std::stringstream ss;
                ss << "webcc_" << d.ns << "_" << d.func_name << ": (";
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (i)
                        ss << ", ";
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    ss << name;
                    if (p.type == "string")
                        ss << "_ptr, " << name << "_len";
                }
                ss << ") => {\n";

                // Decode strings
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    if (p.type == "string")
                    {
                        ss << "                const " << name << " = decoder.decode(new Uint8Array(memory.buffer, " << name << "_ptr, " << name << "_len));\n";
                    }
                }

                ss << "                " << d.action << "\n";
                ss << "            }";
                generated_js_imports.push_back(ss.str());
            }
            else
            {
                // For commands without a return value, generate a case in the flush switch.
                cases_builder << gen_js_case(d);
            }

            used_namespaces.insert(d.ns);
            auto maps = get_maps_from_action(d.action);
            for (const auto &m : maps)
                used_maps.insert(m);
        }
    }

    // Emit resource maps (e.g., for DOM elements, canvases) if they are used.
    if (used_maps.count("elements"))
        js_builder << "    const elements = []; elements[0] = document.body;\n";
    if (used_maps.count("contexts"))
        js_builder << "    const contexts = [];\n";
    if (used_maps.count("audios"))
        js_builder << "    const audios = [];\n";
    if (used_maps.count("websockets"))
        js_builder << "    const websockets = [];\n";
    if (used_maps.count("images"))
        js_builder << "    const images = [];\n";
    if (used_maps.count("webgl_contexts"))
        js_builder << "    const webgl_contexts = [];\n";
    if (used_maps.count("webgl_shaders"))
        js_builder << "    const webgl_shaders = [];\n";
    if (used_maps.count("webgl_programs"))
        js_builder << "    const webgl_programs = [];\n";
    if (used_maps.count("webgl_buffers"))
        js_builder << "    const webgl_buffers = [];\n";
    if (used_maps.count("textures"))
        js_builder << "    const textures = [];\n";
    if (used_maps.count("webgl_uniforms"))
        js_builder << "    const webgl_uniforms = [];\n";
    if (used_maps.count("webgpu_adapters"))
        js_builder << "    const webgpu_adapters = [];\n";
    if (used_maps.count("webgpu_devices"))
        js_builder << "    const webgpu_devices = [];\n";
    if (used_maps.count("webgpu_queues"))
        js_builder << "    const webgpu_queues = [];\n";
    if (used_maps.count("webgpu_shaders"))
        js_builder << "    const webgpu_shaders = [];\n";
    if (used_maps.count("webgpu_encoders"))
        js_builder << "    const webgpu_encoders = [];\n";
    if (used_maps.count("webgpu_contexts"))
        js_builder << "    const webgpu_contexts = [];\n";
    if (used_maps.count("webgpu_views"))
        js_builder << "    const webgpu_views = [];\n";
    if (used_maps.count("webgpu_passes"))
        js_builder << "    const webgpu_passes = [];\n";
    if (used_maps.count("webgpu_buffers"))
        js_builder << "    const webgpu_buffers = [];\n";
    if (used_maps.count("webgpu_pipelines"))
        js_builder << "    const webgpu_pipelines = [];\n";

    // Assemble the final JavaScript file.
    std::stringstream final_js;
    final_js << JS_INIT_HEAD;
    for (const auto &imp : generated_js_imports)
    {
        final_js << ",\n            " << imp;
    }
    final_js << JS_INIT_TAIL;

    // Event System Setup: Set up buffers for JS to send events to C++.
    final_js << "    const { webcc_event_buffer_ptr, webcc_event_offset_ptr, webcc_event_buffer_capacity } = mod.instance.exports;\n";
    final_js << "    const event_buffer_ptr_val = webcc_event_buffer_ptr();\n";
    final_js << "    const event_offset_ptr_val = webcc_event_offset_ptr();\n";
    final_js << "    let event_offset_view = new Uint32Array(memory.buffer, event_offset_ptr_val, 1);\n";
    final_js << "    let event_view = new DataView(memory.buffer, event_buffer_ptr_val);\n";
    final_js << "    const text_encoder = new TextEncoder();\n";
    final_js << "    const EVENT_BUFFER_SIZE = webcc_event_buffer_capacity();\n\n";

    // Generate push_event helpers in JS for each event type.
    for (const auto &d : defs.events)
    {
        if (used_namespaces.find(d.ns) == used_namespaces.end())
            continue;

        final_js << "    function push_event_" << d.ns << "_" << d.name << "(";
        for (size_t i = 0; i < d.params.size(); ++i)
        {
            if (i)
                final_js << ", ";
            final_js << (d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name);
        }
        final_js << ") {\n";
        final_js << "        if (event_view.buffer !== memory.buffer) {\n";
        final_js << "            event_view = new DataView(memory.buffer, event_buffer_ptr_val);\n";
        final_js << "            event_offset_view = new Uint32Array(memory.buffer, event_offset_ptr_val, 1);\n";
        final_js << "        }\n";
        final_js << "        if (event_offset_view[0] + 4096 > EVENT_BUFFER_SIZE) { console.warn('WebCC: Event buffer full, dropping event " << d.name << "'); return; }\n";
        final_js << "        let pos = event_offset_view[0];\n";
        final_js << "        const start_pos = pos;\n";
        final_js << "        event_view.setUint8(pos, " << (int)d.opcode << "); pos += 1;\n";
        final_js << "        pos += 2; // Skip size\n";
        for (size_t i = 0; i < d.params.size(); ++i)
        {
            const auto &p = d.params[i];
            std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
            if (p.type == "int32")
                final_js << "        event_view.setInt32(pos, " << name << ", true); pos += 4;\n";
            else if (p.type == "uint32")
                final_js << "        event_view.setUint32(pos, " << name << ", true); pos += 4;\n";
            else if (p.type == "float32")
                final_js << "        event_view.setFloat32(pos, " << name << ", true); pos += 4;\n";
            else if (p.type == "string")
            {
                final_js << "        const encoded_" << i << " = text_encoder.encode(" << name << ");\n";
                final_js << "        const len_" << i << " = encoded_" << i << ".length;\n";
                final_js << "        event_view.setUint16(pos, len_" << i << " + 1, true); pos += 2;\n";
                final_js << "        new Uint8Array(memory.buffer, event_buffer_ptr_val + pos).set(encoded_" << i << ");\n";
                final_js << "        event_view.setUint8(pos + len_" << i << ", 0);\n";
                final_js << "        pos += len_" << i << " + 1;\n";
            }
        }
        final_js << "        const len = pos - start_pos;\n";
        final_js << "        event_view.setUint16(start_pos + 1, len, true);\n";
        final_js << "        event_offset_view[0] = pos;\n";
        final_js << "    }\n";
    }

    final_js << js_builder.str();
    final_js << JS_FLUSH_HEAD;
    final_js << cases_builder.str();
    final_js << JS_TAIL;
    write_file(out_dir + "/app.js", final_js.str());
    std::cout << "[WebCC] Generated " << out_dir << "/app.js" << std::endl;

    // C. GENERATE HTML (Basic scaffolding)
    std::string html = R"(
<!DOCTYPE html>
<html>
<body style="margin:0; overflow:hidden;">
    <script src="app.js"></script>
</body>
</html>
)";
    write_file(out_dir + "/index.html", html);
    std::cout << "[WebCC] Generated " << out_dir << "/index.html" << std::endl;

    // D. COMPILE C++ TO WASM (Incremental)
    std::cout << "[WebCC] Compiling..." << std::endl;

    // Ensure cache directory exists
    mkdir(build_dir.c_str(), 0755);

    std::string exe_dir = get_executable_dir();
    std::vector<std::string> all_sources = input_files;
    
    // Add internal sources using absolute paths relative to the compiler executable
    all_sources.push_back(exe_dir + "/src/core/command_buffer.cc");
    all_sources.push_back(exe_dir + "/src/core/event_buffer.cc");

    std::string object_files_str;
    bool compilation_failed = false;

    for (const auto &src : all_sources)
    {
        // Generate a unique object file name based on the source path.
        // We replace non-alphanumeric chars with '_' to flatten the path.
        std::string obj_name = src;
        for (char &c : obj_name)
        {
            if (!isalnum(c)) c = '_';
        }
        std::string obj = build_dir + "/" + obj_name + ".o";

        struct stat src_stat, obj_stat;
        bool need_compile = true;

        if (stat(src.c_str(), &src_stat) == 0)
        {
            if (stat(obj.c_str(), &obj_stat) == 0)
            {
                // If obj is newer than src, we don't need to recompile
                if (obj_stat.st_mtime >= src_stat.st_mtime)
                {
                    need_compile = false;
                }
            }
        }
        else
        {
            std::cerr << "[WebCC] Error: Source file not found: " << src << std::endl;
            return 1;
        }

        if (need_compile)
        {
            std::cout << "  [CC] " << src << std::endl;
            // Compile to object file
            // -c : Compile and assemble, but do not link
            std::string cc_cmd = "clang++ --target=wasm32 -O3 -std=c++20 -nostdlib -c -o " + obj + " " + src + 
                                 " -I " + exe_dir + "/include -I " + exe_dir + "/src/core";
            if (system(cc_cmd.c_str()) != 0)
            {
                compilation_failed = true;
                break;
            }
        }

        object_files_str += obj + " ";
    }

    if (compilation_failed)
    {
        std::cerr << "[WebCC] Compilation failed!" << std::endl;
        return 1;
    }

    std::cout << "[WebCC] Linking..." << std::endl;
    std::string cmd = "clang++ --target=wasm32 -O3 -std=c++20 -nostdlib "
                      "-Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined "
                      "-o " + out_dir + "/app.wasm " +
                      object_files_str;

    // std::cout << "  COMMAND: " << cmd << std::endl;

    int result = system(cmd.c_str());
    if (result != 0)
    {
        std::cerr << "[WebCC] Linking failed!" << std::endl;
        return result;
    }

    std::cout << "[WebCC] Success! Run 'python3 -m http.server' in " << out_dir << " to view." << std::endl;
    return 0;
}
