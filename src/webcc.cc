#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <sys/stat.h>
#include <set>

// Small helpers used by the CLI to read and write files
static std::string read_file(const std::string& path){
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if(!in) return std::string();
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool write_file(const std::string& path, const std::string& contents){
    // Ensure parent directories exist roughly (best-effort)
    size_t pos = path.find_last_of("/");
    if(pos != std::string::npos){
        std::string dir = path.substr(0, pos);
        // try creating directory; ignore errors
        mkdir(dir.c_str(), 0755);
    }
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if(!out) return false;
    out << contents;
    return out.good();
}

// -----------------------------------------------------------------------------
// 1. The Javascript Templates
// -----------------------------------------------------------------------------

const std::string JS_INIT_HEAD = R"(
const run = async () => {
    const response = await fetch('app.wasm');
    const bytes = await response.arrayBuffer();
    const mod = await WebAssembly.instantiate(bytes, {
        env: {
            // C++ calls this function to tell JS "I wrote commands, please execute them"
            webcc_js_flush: (ptr, size) => flush(ptr, size)
)";

const std::string JS_INIT_TAIL = R"(
        }
    });

    // Get exports from WASM
    const { memory, main } = mod.instance.exports;
)";

const std::string JS_FLUSH_HEAD = R"(
    // Reusable text decoder to avoid garbage collection overhead
    const decoder = new TextDecoder();
    let view = new DataView(memory.buffer);
    const string_cache = [];

    function flush(ptr, size) {
        if (size === 0) return;

        if (view.buffer !== memory.buffer) {
            view = new DataView(memory.buffer);
        }

        let pos = ptr;
        const end = ptr + size;
        string_cache.length = 0;

        // Loop through the buffer
        while (pos < end) {
            const opcode = view.getUint8(pos);
            pos += 1;

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

// The Feature Database is driven by `webcc/commands.def`

struct Param {
    std::string type;
    std::string name; // optional; if empty we'll generate argN
};

struct CommandDef {
    std::string ns; // Namespace
    std::string name; // NAME token
    uint8_t opcode;
    std::string func_name; // C++ function name to search for
    std::vector<Param> params; // list of parameters
    std::string action; // JS action body (using arg0.. or custom names)
    std::string return_type; // Optional return type
};

static std::vector<CommandDef> load_command_defs(const std::string& path){
    std::vector<CommandDef> out;
    std::string contents = read_file(path);
    std::istringstream ss(contents);
    std::string line;
    uint8_t current_opcode = 1;

    while(std::getline(ss, line)){
        // trim
        size_t p = line.find_first_not_of(" \t\r\n");
        if(p==std::string::npos) continue;
        if(line[p] == '#') continue;
        // split by '|'
        auto parts = std::vector<std::string>();
        size_t start = 0;
        // We expect 4 separators for 5 parts
        for(int i=0; i<4; ++i){
            size_t pos = line.find('|', start);
            if(pos == std::string::npos){
                break;
            }
            parts.push_back(line.substr(start, pos - start));
            start = pos + 1;
        }
        // The rest is the action
        parts.push_back(line.substr(start));

        if(parts.size() < 5) continue; // malformed (now 5 parts)
        CommandDef c;
        c.ns = parts[0];
        c.name = parts[1];
        c.opcode = current_opcode++;
        c.func_name = parts[2];
        // types are space-separated tokens that can be "type" or "type:name"
        std::istringstream tss(parts[3]);
        std::string tkn;
        while(tss >> tkn){
            Param p;
            size_t colon = tkn.find(':');
            if(colon == std::string::npos){
                p.type = tkn;
                p.name = ""; // will later become argN
            } else {
                p.type = tkn.substr(0, colon);
                p.name = tkn.substr(colon+1);
            }
            
            if (p.type == "RET") {
                c.return_type = p.name;
            } else {
                c.params.push_back(p);
            }
        }
        c.action = parts[4];
        out.push_back(c);
    }
    return out;
}

static std::string gen_js_case(const CommandDef& c){
    std::stringstream ss;
    ss << "            case " << (int)c.opcode << ": {\n";
    // Declare typed variables using the parameter names from the def file
    for(size_t i=0;i<c.params.size();++i){
        const auto& p = c.params[i];
        std::string varName = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
        if(p.type == "uint8"){
            ss << "                const "<<varName<<" = view.getUint8(pos); pos += 1;\n";
        } else if(p.type == "uint32"){
            ss << "                const "<<varName<<" = view.getUint32(pos, true); pos += 4;\n";
        } else if(p.type == "int32"){
            ss << "                const "<<varName<<" = view.getInt32(pos, true); pos += 4;\n";
        } else if(p.type == "float32"){
            ss << "                const "<<varName<<" = view.getFloat32(pos, true); pos += 4;\n";
        } else if(p.type == "string"){
            ss << "                const "<<varName<<"_tag = view.getUint8(pos); pos += 1;\n";
            ss << "                let "<<varName<<";\n";
            ss << "                if ("<<varName<<"_tag === 0) {\n";
            ss << "                    const "<<varName<<"_id = view.getUint16(pos, true); pos += 2;\n";
            ss << "                    "<<varName<<" = string_cache["<<varName<<"_id];\n";
            ss << "                } else {\n";
            ss << "                    const "<<varName<<"_len = view.getUint16(pos, true); pos += 2;\n";
            ss << "                    "<<varName<<" = decoder.decode(new Uint8Array(memory.buffer, pos, "<<varName<<"_len)); pos += "<<varName<<"_len;\n";
            ss << "                    string_cache.push("<<varName<<");\n";
            ss << "                }\n";
        } else {
            ss << "                // Unknown type: "<<p.type<<"\n";
        }
    }
    ss << "                " << c.action << "\n";
    ss << "                break;\n            }\n";
    return ss.str();
}

// Emit a generated C++ header to `include/webcc/commands_gen.h` so C++ API matches defs
static void emit_command_headers(const std::vector<CommandDef>& defs){
    // Emit per-namespace headers
    std::vector<std::string> namespaces;
    for(const auto& d : defs){
        bool found = false;
        for(const auto& ns : namespaces) if(ns == d.ns) found = true;
        if(!found) namespaces.push_back(d.ns);
    }

    for(const auto& ns : namespaces){
        std::stringstream out;
        out << "#pragma once\n";
        out << "#include \"webcc.h\"\n\n";
        out << "namespace webcc::" << ns << " {\n";
        out << "    enum OpCode {\n";
        bool first = true;
        for(const auto& d : defs){
            if(d.ns != ns) continue;
            if(!first) out << ",\n";
            out << "        OP_"<<d.name<<" = 0x"<< std::hex << (int)d.opcode << std::dec;
            first = false;
        }
        out << "\n    };\n\n";

        // Functions
        for(const auto& d : defs){
            if(d.ns != ns) continue;

            if (!d.return_type.empty()) {
                std::string ret_type = d.return_type;
                if(ret_type == "int32") ret_type = "int32_t";
                else if(ret_type == "uint32") ret_type = "uint32_t";
                else if(ret_type == "float32") ret_type = "float";

                // Generate extern "C" import
                out << "    extern \"C\" " << ret_type << " webcc_" << d.ns << "_" << d.func_name << "(";
                for(size_t i=0;i<d.params.size();++i){
                    if(i) out << ", ";
                    const auto& p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    if(p.type=="string") out << "const char* "<<name << ", uint32_t " << name << "_len";
                    else if(p.type=="float32") out << "float "<<name;
                    else if(p.type=="uint8") out << "uint8_t "<<name;
                    else if(p.type=="uint32") out << "uint32_t "<<name;
                    else if(p.type=="int32") out << "int32_t "<<name;
                    else out << "/*unknown*/ void* "<<name;
                }
                out << ");\n";
                
                // Generate inline wrapper
                out << "    inline " << ret_type << " " << d.func_name << "(";
                for(size_t i=0;i<d.params.size();++i){
                    if(i) out << ", ";
                    const auto& p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    if(p.type=="string") out << "const char* "<<name;
                    else if(p.type=="float32") out << "float "<<name;
                    else if(p.type=="uint8") out << "uint8_t "<<name;
                    else if(p.type=="uint32") out << "uint32_t "<<name;
                    else if(p.type=="int32") out << "int32_t "<<name;
                    else out << "/*unknown*/ void* "<<name;
                }
                out << "){\n";
                out << "        ::webcc::flush();\n";
                out << "        return webcc_" << d.ns << "_" << d.func_name << "(";
                for(size_t i=0;i<d.params.size();++i){
                    if(i) out << ", ";
                    const auto& p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    out << name;
                    if(p.type=="string") out << ", webcc::strlen(" << name << ")";
                }
                out << ");\n";
                out << "    }\n\n";
                continue;
            }

            out << "    inline void "<<d.func_name<<"(";
            // param list
            for(size_t i=0;i<d.params.size();++i){
                if(i) out << ", ";
                const auto& p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                if(p.type=="uint8") out << "uint8_t "<<name;
                else if(p.type=="uint32") out << "uint32_t "<<name;
                else if(p.type=="int32") out << "int32_t "<<name;
                else if(p.type=="float32") out << "float "<<name;
                else if(p.type=="string") out << "const char* "<<name;
                else out << "/*unknown*/ void* "<<name;
            }
            out << "){\n";
            out << "        push_command((uint8_t)OP_"<<d.name<<");\n";
            for(size_t i=0;i<d.params.size();++i){
                const auto& p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                if(p.type=="uint8") out << "        push_data<uint8_t>("<<name<<");\n";
                else if(p.type=="uint32") out << "        push_data<uint32_t>("<<name<<");\n";
                else if(p.type=="int32") out << "        push_data<int32_t>("<<name<<");\n";
                else if(p.type=="float32") out << "        push_data<float>("<<name<<");\n";
                else if(p.type=="string") out << "        webcc::CommandBuffer::push_string("<<name<<", strlen("<<name<<"));\n";
                else out << "        // unknown type: "<<p.type<<"\n";
            }
            out << "    }\n\n";
        }
        out << "} // namespace webcc::" << ns << "\n\n";
        
        write_file("webcc/include/webcc/" + ns + ".h", out.str());
        std::cout << "[WebCC] Emitted include/webcc/" << ns << ".h" << std::endl;
    }
}

// Helper to check if 'text' contains 'word' as a whole identifier
static bool contains_whole_word(const std::string& text, const std::string& word) {
    size_t pos = 0;
    while ((pos = text.find(word, pos)) != std::string::npos) {
        // Check character before
        bool boundary_start = (pos == 0);
        if (!boundary_start) {
            char c = text[pos - 1];
            if (isalnum(c) || c == '_') boundary_start = false;
            else boundary_start = true;
        }

        // Check character after
        bool boundary_end = (pos + word.length() == text.length());
        if (!boundary_end) {
            char c = text[pos + word.length()];
            if (isalnum(c) || c == '_') boundary_end = false;
            else boundary_end = true;
        }

        if (boundary_start && boundary_end) return true;
        
        pos += 1;
    }
    return false;
}

static std::set<std::string> get_maps_from_action(const std::string& action) {
    std::set<std::string> maps;
    std::vector<std::string> possible_maps = {"elements", "canvases", "audios", "websockets", "images", "contexts", "shaders", "programs", "buffers", "textures", "uniforms"};
    for (const auto& map : possible_maps) {
        if (contains_whole_word(action, map)) {
            maps.insert(map);
        }
    }
    return maps;
}

int main(int argc, char** argv){
    if (argc < 2) {
        std::cerr << "Usage: ./webcc <file1.cc> [file2.cc ...]" << std::endl;
        return 1;
    }

    std::string user_code;
    std::string source_files;

    // A. READ USER CODE (All files)
    for(int i=1; i<argc; ++i) {
        std::string path = argv[i];
        std::string content = read_file(path);
        if (content.empty()) {
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
    auto defs = load_command_defs("webcc/commands.def");
    emit_command_headers(defs);

    std::set<std::string> used_namespaces;
    std::set<std::string> used_maps;
    std::stringstream cases_builder;
    std::vector<std::string> generated_js_imports;

    for(const auto& d : defs){
        bool used = false;
        // 1. Check for qualified usage: ns::func or webcc::ns::func
        if (contains_whole_word(user_code, d.ns + "::" + d.func_name)) used = true;
        else if (contains_whole_word(user_code, "webcc::" + d.ns + "::" + d.func_name)) used = true;
        
        // 2. Check for using namespace
        if (!used) {
            bool ns_used = contains_whole_word(user_code, "using namespace webcc::" + d.ns) || 
                           contains_whole_word(user_code, "using namespace webcc");
            if (ns_used && contains_whole_word(user_code, d.func_name)) used = true;
        }

        if(used){
            std::cout << "  -> Found " << d.func_name << " (" << d.ns << "), embedding JS support." << std::endl;
            
            if (!d.return_type.empty()) {
                std::stringstream ss;
                ss << "webcc_" << d.ns << "_" << d.func_name << ": (";
                for(size_t i=0;i<d.params.size();++i){
                    if(i) ss << ", ";
                    const auto& p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    ss << name;
                    if (p.type == "string") ss << "_ptr, " << name << "_len";
                }
                ss << ") => {\n";
                
                // Decode strings
                for(size_t i=0;i<d.params.size();++i){
                    const auto& p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    if (p.type == "string") {
                        ss << "                const " << name << " = decoder.decode(new Uint8Array(memory.buffer, " << name << "_ptr, " << name << "_len));\n";
                    }
                }
                
                ss << "                " << d.action << "\n";
                ss << "            }";
                generated_js_imports.push_back(ss.str());
            } else {
                cases_builder << gen_js_case(d);
            }

            used_namespaces.insert(d.ns);
            auto maps = get_maps_from_action(d.action);
            for (const auto& m : maps) used_maps.insert(m);
        }
    }

    // Emit resource maps
    if (used_maps.count("elements")) js_builder << "    const elements = new Map(); elements.set('body', document.body);\n";
    if (used_maps.count("canvases")) js_builder << "    const canvases = new Map();\n";
    if (used_maps.count("audios")) js_builder << "    const audios = new Map();\n";
    if (used_maps.count("websockets")) js_builder << "    const websockets = new Map();\n";
    if (used_maps.count("images")) js_builder << "    const images = new Map();\n";
    if (used_maps.count("contexts")) js_builder << "    const contexts = new Map();\n";
    if (used_maps.count("shaders")) js_builder << "    const shaders = new Map();\n";
    if (used_maps.count("programs")) js_builder << "    const programs = new Map();\n";
    if (used_maps.count("buffers")) js_builder << "    const buffers = new Map();\n";
    if (used_maps.count("textures")) js_builder << "    const textures = new Map();\n";
    if (used_maps.count("uniforms")) js_builder << "    const uniforms = new Map();\n";

    std::stringstream final_js;
    final_js << JS_INIT_HEAD;
    for(const auto& imp : generated_js_imports) {
        final_js << ",\n            " << imp;
    }
    final_js << JS_INIT_TAIL;
    final_js << js_builder.str();
    final_js << JS_FLUSH_HEAD;
    final_js << cases_builder.str();
    final_js << JS_TAIL;
    write_file("app.js", final_js.str());
    std::cout << "[WebCC] Generated app.js" << std::endl;

    // C. GENERATE HTML (Basic scaffolding)
    std::string html = R"(
<!DOCTYPE html>
<html>
<body style="margin:0; overflow:hidden;">
    <script src="app.js"></script>
</body>
</html>
)";
    write_file("index.html", html);
    std::cout << "[WebCC] Generated index.html" << std::endl;

    // D. COMPILE C++ TO WASM
    // We construct the clang command here.
    // Flags explanation:
    // --target=wasm32       : Output WebAssembly
    // -nostdlib             : Don't link standard libc (keeps it tiny)
    // -Wl,--no-entry        : We don't have a standard C main() entry point immediately
    // -Wl,--export-all      : Export our functions (webcc_get_buffer, etc) to JS
    // -Wl,--allow-undefined : Allow 'webcc_js_flush' to be undefined (JS provides it)
    // Add both the top-level `include` (generated headers) and the
    // `webcc/include` path (packaged headers) so user code can include
    // either `webcc/...` or generated `include/...` headers.
    std::string cmd = "clang++ --target=wasm32 -O3 -nostdlib "
                      "-Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined "
                      "-o app.wasm "
                      + source_files + " webcc/src/command_buffer.cc -I include -I webcc/include";

    std::cout << "[WebCC] Compiling WASM..." << std::endl;
    std::cout << "  COMMAND: " << cmd << std::endl;

    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "[WebCC] Compilation failed!" << std::endl;
        return result;
    }

    std::cout << "[WebCC] Success! Run 'python3 -m http.server' to view." << std::endl;
    return 0;
}
