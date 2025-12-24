#include "generators.h"
#include "utils.h"
#include "js_templates.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cctype>
#include <set>
#include <sys/stat.h>

// Helper to map schema types to C++ types
static std::string map_cpp_type(const std::string& type, const std::string& name) {
    if (type == "string") return "webcc::string_view";
    if ((type == "int32" || type == "uint32") && 
        (name.find("handle") != std::string::npos || name == "id" || name.find("_id") != std::string::npos)) {
        return "webcc::handle";
    }
    if (type == "int32") return "int32_t";
    if (type == "uint32") return "uint32_t";
    if (type == "float32") return "float";
    if (type == "uint8") return "uint8_t";
    if (type == "func_ptr") return "void*";
    return "void*";
}

void emit_schema_header(const Defs &defs)
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
    out << "    SchemaParam params[16];\n";
    out << "};\n\n";

    out << "struct SchemaEvent {\n";
    out << "    const char* ns;\n";
    out << "    const char* name;\n";
    out << "    uint8_t opcode;\n";
    out << "    int num_params;\n";
    out << "    SchemaParam params[16];\n";
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

void emit_headers(const Defs &defs)
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
        out << "#include \"webcc.h\"\n";
        out << "#include \"webcc/core/handle.h\"\n";
        out << "#include \"webcc/core/string_view.h\"\n\n";
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
                    std::string type = map_cpp_type(p.type, p.name);
                    std::string name = p.name;
                    out << "        " << type << " " << name << ";\n";
                }

                out << "\n        static " << struct_name << " parse(const uint8_t* data, uint32_t len) {\n";
                out << "            " << struct_name << " res;\n";
                out << "            uint32_t offset = 0;\n";
                for (const auto &p : d.params) {
                    std::string cpp_type = map_cpp_type(p.type, p.name);
                    if (p.type == "int32") {
                        if (cpp_type == "webcc::handle")
                            out << "            res." << p.name << " = webcc::handle(*(int32_t*)(data + offset)); offset += 4;\n";
                        else
                            out << "            res." << p.name << " = *(int32_t*)(data + offset); offset += 4;\n";
                    } else if (p.type == "uint32") {
                        if (cpp_type == "webcc::handle")
                            out << "            res." << p.name << " = webcc::handle((int32_t)*(uint32_t*)(data + offset)); offset += 4;\n";
                        else
                            out << "            res." << p.name << " = *(uint32_t*)(data + offset); offset += 4;\n";
                    } else if (p.type == "float32") {
                        out << "            res." << p.name << " = *(float*)(data + offset); offset += 4;\n";
                    } else if (p.type == "uint8") {
                        out << "            res." << p.name << " = *(uint8_t*)(data + offset); offset += 4;\n";
                    } else if (p.type == "string") {
                        out << "            uint32_t " << p.name << "_len = *(uint32_t*)(data + offset); offset += 4;\n";
                        out << "            res." << p.name << " = webcc::string_view((const char*)(data + offset), " << p.name << "_len);\n";
                        out << "            offset += (" << p.name << "_len + 3) & ~3;\n";
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
                std::string wrapper_ret_type = ret_type;
                bool ret_is_handle = (ret_type == "int32_t");
                if (ret_is_handle) wrapper_ret_type = "webcc::handle";

                out << "    inline " << wrapper_ret_type << " " << d.func_name << "(";
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (i)
                        out << ", ";
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    out << map_cpp_type(p.type, p.name) << " " << name;
                }
                out << "){\n";
                out << "        ::webcc::flush();\n";
                out << "        return ";
                if (ret_is_handle) out << "webcc::handle(";
                out << "webcc_" << d.ns << "_" << d.func_name << "(";
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (i)
                        out << ", ";
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    std::string cpp_type = map_cpp_type(p.type, p.name);
                    
                    if (cpp_type == "webcc::string_view") {
                        out << name << ".data(), " << name << ".length()";
                    } else if (cpp_type == "webcc::handle") {
                        out << "(int32_t)" << name;
                    } else {
                        out << name;
                    }
                }
                out << ")";
                if (ret_is_handle) out << ")";
                out << ";\n";
                out << "    }\n\n";
                continue;
            }

            // Check if we need templates for func_ptr
            std::vector<std::string> t_params;
            for (size_t i = 0; i < d.params.size(); ++i) {
                if (d.params[i].type == "func_ptr") {
                    std::string pname = d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name;
                    std::string tname = "T_" + pname;
                    t_params.push_back(tname);
                }
            }

            if (!t_params.empty()) {
                out << "    template <";
                for (size_t i = 0; i < t_params.size(); ++i) {
                    if (i > 0) out << ", ";
                    out << "typename " << t_params[i];
                }
                out << ">\n";
            }

            out << "    inline void " << d.func_name << "(";
            // param list
            int t_idx = 0;
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                if (i)
                    out << ", ";
                const auto &p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                if (p.type == "func_ptr") {
                    out << t_params[t_idx++] << " " << name;
                } else {
                    out << map_cpp_type(p.type, p.name) << " " << name;
                }
            }
            out << "){\n";
            out << "        push_command((uint32_t)OP_" << d.name << ");\n";
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                const auto &p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                std::string cpp_type = map_cpp_type(p.type, p.name);

                if (cpp_type == "webcc::string_view")
                    out << "        webcc::CommandBuffer::push_string(" << name << ".data(), " << name << ".length());\n";
                else if (cpp_type == "webcc::handle")
                    out << "        push_data<int32_t>((int32_t)" << name << ");\n";
                else if (p.type == "uint8")
                    out << "        push_data<uint32_t>((uint32_t)" << name << ");\n";
                else if (p.type == "uint32")
                    out << "        push_data<uint32_t>(" << name << ");\n";
                else if (p.type == "int32")
                    out << "        push_data<int32_t>(" << name << ");\n";
                else if (p.type == "float32")
                    out << "        push_data<float>(" << name << ");\n";
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

std::string gen_js_case(const CommandDef &c)
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

bool contains_whole_word(const std::string &text, const std::string &word)
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

static const std::vector<std::string> RESOURCE_MAPS = {
    "elements", "contexts", "audios", "websockets", "images",
    "webgl_contexts", "webgl_shaders", "webgl_programs", "webgl_buffers",
    "textures", "webgl_uniforms",
    "webgpu_adapters", "webgpu_devices", "webgpu_queues", "webgpu_shaders",
    "webgpu_encoders", "webgpu_contexts", "webgpu_views", "webgpu_passes",
    "webgpu_buffers", "webgpu_pipelines"
};

std::set<std::string> get_maps_from_action(const std::string &action)
{
    std::set<std::string> maps;
    for (const auto &map : RESOURCE_MAPS)
    {
        if (contains_whole_word(action, map))
        {
            maps.insert(map);
        }
    }
    return maps;
}

void generate_js_runtime(const Defs &defs, const std::string &user_code, const std::string &out_dir)
{
    std::stringstream js_builder;

    std::cout << "[WebCC] Scanning source files for features..." << std::endl;

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
    for (const auto &map : RESOURCE_MAPS)
    {
        if (used_maps.count(map))
        {
            js_builder << "    const " << map << " = [];";
            if (map == "elements")
                js_builder << " elements[0] = document.body;";
            js_builder << "\n";
        }
    }

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
    final_js << "    let event_u8 = new Uint8Array(memory.buffer, event_buffer_ptr_val);\n";
    final_js << "    let event_i32 = new Int32Array(memory.buffer, event_buffer_ptr_val);\n";
    final_js << "    let event_f32 = new Float32Array(memory.buffer, event_buffer_ptr_val);\n";
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
        final_js << "        if (event_u8.buffer !== memory.buffer) {\n";
        final_js << "            event_u8 = new Uint8Array(memory.buffer, event_buffer_ptr_val);\n";
        final_js << "            event_i32 = new Int32Array(memory.buffer, event_buffer_ptr_val);\n";
        final_js << "            event_f32 = new Float32Array(memory.buffer, event_buffer_ptr_val);\n";
        final_js << "            event_offset_view = new Uint32Array(memory.buffer, event_offset_ptr_val, 1);\n";
        final_js << "        }\n";
        final_js << "        if (event_offset_view[0] + 4096 > EVENT_BUFFER_SIZE) { console.warn('WebCC: Event buffer full, dropping event " << d.name << "'); return; }\n";
        final_js << "        let pos = event_offset_view[0];\n";
        final_js << "        const start_pos = pos;\n";
        final_js << "        event_u8[pos] = " << (int)d.opcode << ";\n";
        final_js << "        pos += 4; // Skip header (opcode + pad + size)\n";
        for (size_t i = 0; i < d.params.size(); ++i)
        {
            const auto &p = d.params[i];
            std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
            if (p.type == "int32")
                final_js << "        event_i32[pos >> 2] = " << name << "; pos += 4;\n";
            else if (p.type == "uint32")
                final_js << "        event_i32[pos >> 2] = " << name << "; pos += 4;\n";
            else if (p.type == "uint8")
                final_js << "        event_i32[pos >> 2] = " << name << "; pos += 4;\n";
            else if (p.type == "float32")
                final_js << "        event_f32[pos >> 2] = " << name << "; pos += 4;\n";
            else if (p.type == "string")
            {
                final_js << "        const encoded_" << i << " = text_encoder.encode(" << name << ");\n";
                final_js << "        const len_" << i << " = encoded_" << i << ".length;\n";
                final_js << "        event_i32[pos >> 2] = len_" << i << "; pos += 4;\n";
                final_js << "        new Uint8Array(memory.buffer, event_buffer_ptr_val + pos).set(encoded_" << i << ");\n";
                final_js << "        pos += (len_" << i << " + 3) & ~3;\n";
            }
        }
        final_js << "        const len = pos - start_pos;\n";
        final_js << "        event_u8[start_pos + 2] = len & 0xFF;\n";
        final_js << "        event_u8[start_pos + 3] = (len >> 8) & 0xFF;\n";
        final_js << "        event_offset_view[0] = pos;\n";
        final_js << "    }\n";
    }

    final_js << js_builder.str();
    final_js << JS_FLUSH_HEAD;
    final_js << cases_builder.str();
    final_js << JS_TAIL;
    write_file(out_dir + "/app.js", final_js.str());
    std::cout << "[WebCC] Generated " << out_dir << "/app.js" << std::endl;
}

void generate_html(const std::string &out_dir)
{
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
}

bool compile_wasm(const std::vector<std::string> &input_files, const std::string &out_dir, const std::string &build_dir)
{
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
            return false;
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
        return false;
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
        return false;
    }

    std::cout << "[WebCC] Success! Run 'python3 -m http.server' in " << out_dir << " to view." << std::endl;
    return true;
}
