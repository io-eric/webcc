#include "generators.h"
#include "utils.h"
#include "js_templates.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cctype>
#include <set>
#include <sys/stat.h>

namespace webcc
{

    // Helper to map schema types to C++ types
    static std::string map_cpp_type(const std::string &type, const std::string &name)
    {
        if (type == "string")
            return "webcc::string_view";
        if ((type == "int32" || type == "uint32") &&
            (name.find("handle") != std::string::npos || name == "id" || name.find("_id") != std::string::npos))
        {
            return "webcc::handle";
        }
        if (type == "int32")
            return "int32_t";
        if (type == "uint32")
            return "uint32_t";
        if (type == "float32")
            return "float";
        if (type == "uint8")
            return "uint8_t";
        if (type == "func_ptr")
            return "void*";
        return "void*";
    }

    void emit_schema_header(const SchemaDefs &defs)
    {
        CodeWriter w;
        w.write(R"(// GENERATED FILE - DO NOT EDIT
#pragma once
#include "schema.h"
#include <cstdint>

namespace webcc {
)");

        w.write("static const SchemaCommand SCHEMA_COMMANDS[] = {");
        for (const auto &d : defs.commands)
        {
            std::stringstream ss;
            ss << "{ \"" << d.ns << "\", \"" << d.name << "\", " << (int)d.opcode << ", ";
            ss << "\"" << d.func_name << "\", {";
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                if (i > 0)
                    ss << ", ";
                std::string name = d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name;
                ss << "{ \"" << d.params[i].type << "\", \"" << name << "\" }";
            }
            ss << "}, ";
            ss << "R\"JS_ACTION(" << d.action << ")JS_ACTION\", ";
            ss << "\"" << d.return_type << "\" },";
            w.write(ss.str());
        }
        w.write("{ \"\", \"\", 0, \"\", {}, \"\", \"\" }");
        w.write("};");
        w.write("");

        w.write("static const SchemaEvent SCHEMA_EVENTS[] = {");
        for (const auto &d : defs.events)
        {
            std::stringstream ss;
            ss << "{ \"" << d.ns << "\", \"" << d.name << "\", " << (int)d.opcode << ", {";
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                if (i > 0)
                    ss << ", ";
                std::string name = d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name;
                ss << "{ \"" << d.params[i].type << "\", \"" << name << "\" }";
            }
            ss << "} },";
            w.write(ss.str());
        }
        w.write("{ \"\", \"\", 0, {} }");
        w.write("};");
        w.write("");

        w.write("} // namespace webcc");

        write_file("src/cli/webcc_schema.h", w.str());
        std::cout << "[WebCC] Emitted src/cli/webcc_schema.h" << std::endl;
    }

    void emit_headers(const SchemaDefs &defs)
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
            CodeWriter w;
            w.write("// GENERATED FILE - DO NOT EDIT");
            w.write("#pragma once");
            w.write("#include \"webcc.h\"");
            w.write("#include \"webcc/core/handle.h\"");
            w.write("#include \"webcc/core/string_view.h\"");
            w.write("#include \"webcc/core/string.h\"");
            w.write("namespace webcc::" + ns + " {");

            // Commands
            w.write("enum OpCode {");
            for (const auto &d : defs.commands)
            {
                if (d.ns != ns)
                    continue;
                std::string line = std::string("OP_") + d.name + " = 0x";
                std::stringstream ss;
                ss << std::hex << (int)d.opcode << std::dec;
                w.write(line + ss.str() + ",");
            }
            w.write("};");
            w.write("");

            // Events
            bool has_events = false;
            for (const auto &d : defs.events)
                if (d.ns == ns)
                    has_events = true;

            if (has_events)
            {
                w.write("enum EventType {");
                for (const auto &d : defs.events)
                {
                    if (d.ns != ns)
                        continue;
                    std::string line = std::string("EVENT_") + d.name + " = 0x";
                    std::stringstream ss;
                    ss << std::hex << (int)d.opcode << std::dec;
                    w.write(line + ss.str() + ",");
                }
                w.write("};");
                w.write("");

                w.write("enum EventMask {");
                int shift = 0;
                for (const auto &d : defs.events)
                {
                    if (d.ns != ns)
                        continue;
                    w.write(std::string("MASK_") + d.name + " = 1 << " + std::to_string(shift++) + ",");
                }
                w.write("};");
                w.write("");

                // Generate Event Structs
                for (const auto &d : defs.events)
                {
                    if (d.ns != ns)
                        continue;

                    std::string struct_name;
                    bool next_upper = true;
                    for (char c : d.name)
                    {
                        if (c == '_')
                        {
                            next_upper = true;
                        }
                        else
                        {
                            if (next_upper)
                            {
                                struct_name += toupper(c);
                                next_upper = false;
                            }
                            else
                            {
                                struct_name += tolower(c);
                            }
                        }
                    }
                    struct_name += "Event";

                    w.write("struct " + struct_name + " {");
                    for (const auto &p : d.params)
                    {
                        std::string type = map_cpp_type(p.type, p.name);
                        std::string name = p.name;
                        w.write(type + " " + name + ";");
                    }

                    w.write("");
                    w.write("static " + struct_name + " parse(const uint8_t* data, uint32_t len) {");
                    w.write(struct_name + " res;");
                    w.write("uint32_t offset = 0;");
                    for (const auto &p : d.params)
                    {
                        std::string cpp_type = map_cpp_type(p.type, p.name);
                        if (p.type == "int32")
                        {
                            if (cpp_type == "webcc::handle")
                                w.write("res." + p.name + " = webcc::handle(*(int32_t*)(data + offset)); offset += 4;");
                            else
                                w.write("res." + p.name + " = *(int32_t*)(data + offset); offset += 4;");
                        }
                        else if (p.type == "uint32")
                        {
                            if (cpp_type == "webcc::handle")
                                w.write("res." + p.name + " = webcc::handle((int32_t)*(uint32_t*)(data + offset)); offset += 4;");
                            else
                                w.write("res." + p.name + " = *(uint32_t*)(data + offset); offset += 4;");
                        }
                        else if (p.type == "float32")
                        {
                            w.write("res." + p.name + " = *(float*)(data + offset); offset += 4;");
                        }
                        else if (p.type == "uint8")
                        {
                            w.write("res." + p.name + " = *(uint8_t*)(data + offset); offset += 4;");
                        }
                        else if (p.type == "string")
                        {
                            w.write("uint32_t " + p.name + "_len = *(uint32_t*)(data + offset); offset += 4;");
                            w.write("res." + p.name + " = webcc::string_view((const char*)(data + offset), " + p.name + "_len);");
                            w.write("offset += (" + p.name + "_len + 3) & ~3;");
                        }
                    }
                    w.write("return res;");
                    w.write("}");

                    w.write("};");
                    w.write("");
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
                    std::string c_ret_type = ret_type;
                    if (ret_type == "string")
                        c_ret_type = "uint32_t"; // Returns length

                    std::stringstream sig;
                    sig << "extern \"C\" " << c_ret_type << " webcc_" << d.ns << "_" << d.func_name << "(";
                    for (size_t i = 0; i < d.params.size(); ++i)
                    {
                        if (i)
                            sig << ", ";
                        const auto &p = d.params[i];
                        std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                        if (p.type == "string")
                            sig << "const char* " << name << ", uint32_t " << name << "_len";
                        else if (p.type == "float32")
                            sig << "float " << name;
                        else if (p.type == "uint8")
                            sig << "uint8_t " << name;
                        else if (p.type == "uint32")
                            sig << "uint32_t " << name;
                        else if (p.type == "int32")
                            sig << "int32_t " << name;
                        else
                            sig << "/*unknown*/ void* " << name;
                    }
                    sig << ");";
                    w.write(sig.str());

                    // Generate inline wrapper
                    std::string wrapper_ret_type = ret_type;
                    bool ret_is_handle = (ret_type == "int32_t");
                    if (ret_is_handle)
                        wrapper_ret_type = "webcc::handle";
                    if (ret_type == "string")
                        wrapper_ret_type = "webcc::string";

                    std::stringstream wrap;
                    wrap << "inline " << wrapper_ret_type << " " << d.func_name << "(";
                    for (size_t i = 0; i < d.params.size(); ++i)
                    {
                        if (i)
                            wrap << ", ";
                        const auto &p = d.params[i];
                        std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                        wrap << map_cpp_type(p.type, p.name) << " " << name;
                    }
                    wrap << "){";
                    w.write(wrap.str());
                    w.write("::webcc::flush();");

                    std::stringstream call;
                    if (ret_type == "string")
                    {
                        call << "uint32_t len = webcc_" << d.ns << "_" << d.func_name << "(";
                    }
                    else
                    {
                        call << "return ";
                        if (ret_is_handle)
                            call << "webcc::handle(";
                        call << "webcc_" << d.ns << "_" << d.func_name << "(";
                    }

                    for (size_t i = 0; i < d.params.size(); ++i)
                    {
                        if (i)
                            call << ", ";
                        const auto &p = d.params[i];
                        std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                        std::string cpp_type = map_cpp_type(p.type, p.name);

                        if (cpp_type == "webcc::string_view")
                        {
                            call << name << ".data(), " << name << ".length()";
                        }
                        else if (cpp_type == "webcc::handle")
                        {
                            call << "(int32_t)" << name;
                        }
                        else
                        {
                            call << name;
                        }
                    }
                    call << ")";
                    if (ret_is_handle)
                        call << ")";
                    call << ";";
                    w.write(call.str());

                    if (ret_type == "string")
                    {
                        w.write("const char* data = (const char*)::webcc::scratch_buffer_data();");
                        w.write("return webcc::string(data, len);");
                    }

                    w.write("}");
                    w.write("");
                    continue;
                }

                // Check if we need templates for func_ptr
                std::vector<std::string> t_params;
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (d.params[i].type == "func_ptr")
                    {
                        std::string pname = d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name;
                        std::string tname = "T_" + pname;
                        t_params.push_back(tname);
                    }
                }

                if (!t_params.empty())
                {
                    std::stringstream tpl;
                    tpl << "template <";
                    for (size_t i = 0; i < t_params.size(); ++i)
                    {
                        if (i > 0)
                            tpl << ", ";
                        tpl << "typename " << t_params[i];
                    }
                    tpl << ">";
                    w.write(tpl.str());
                }

                std::stringstream func;
                func << "inline void " << d.func_name << "(";
                // param list
                int t_idx = 0;
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    if (i)
                        func << ", ";
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    if (p.type == "func_ptr")
                    {
                        func << t_params[t_idx++] << " " << name;
                    }
                    else
                    {
                        func << map_cpp_type(p.type, p.name) << " " << name;
                    }
                }
                func << "){";
                w.write(func.str());
                w.write("push_command((uint32_t)OP_" + d.name + ");");
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    std::string cpp_type = map_cpp_type(p.type, p.name);

                    if (cpp_type == "webcc::string_view")
                        w.write("webcc::CommandBuffer::push_string(" + name + ".data(), " + name + ".length());");
                    else if (cpp_type == "webcc::handle")
                        w.write("push_data<int32_t>((int32_t)" + name + ");");
                    else if (p.type == "uint8")
                        w.write("push_data<uint32_t>((uint32_t)" + name + ");");
                    else if (p.type == "uint32")
                        w.write("push_data<uint32_t>(" + name + ");");
                    else if (p.type == "int32")
                        w.write("push_data<int32_t>(" + name + ");");
                    else if (p.type == "float32")
                        w.write("push_data<float>(" + name + ");");
                    else if (p.type == "func_ptr")
                        w.write("push_data<uint32_t>((uint32_t)(uintptr_t)" + name + ");");
                    else
                        w.write("// unknown type: " + p.type);
                }
                w.write("}");
                w.write("");
            }
            w.write("} // namespace webcc::" + ns);

            write_file("include/webcc/" + ns + ".h", w.str());
            std::cout << "[WebCC] Emitted include/webcc/" << ns << ".h" << std::endl;
        }
        emit_schema_header(defs);
    }

    void gen_js_case(const SchemaCommand &c, CodeWriter &w)
    {
        w.write("case " + std::to_string((int)c.opcode) + ": {");
        // Declare typed variables using the parameter names from the def file
        for (size_t i = 0; i < c.params.size(); ++i)
        {
            const auto &p = c.params[i];
            std::string varName = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
            if (p.type == "uint8" || p.type == "uint32")
            {
                w.write("if (pos + 4 > end) { console.error('WebCC: OOB " + varName + "'); break; }");
                w.write("const " + varName + " = i32[pos >> 2]; pos += 4;");
            }
            else if (p.type == "int32")
            {
                w.write("if (pos + 4 > end) { console.error('WebCC: OOB " + varName + "'); break; }");
                w.write("const " + varName + " = i32[pos >> 2]; pos += 4;");
            }
            else if (p.type == "float32")
            {
                w.write("if (pos + 4 > end) { console.error('WebCC: OOB " + varName + "'); break; }");
                w.write("const " + varName + " = f32[pos >> 2]; pos += 4;");
            }
            else if (p.type == "func_ptr")
            {
                w.write("if (pos + 4 > end) { console.error('WebCC: OOB " + varName + "'); break; }");
                w.write("const " + varName + " = i32[pos >> 2]; pos += 4;");
            }
            else if (p.type == "string")
            {
                w.write("if (pos + 4 > end) { console.error('WebCC: OOB " + varName + "_len'); break; }");
                w.write("const " + varName + "_len = i32[pos >> 2]; pos += 4;");
                w.write("const " + varName + "_padded = (" + varName + "_len + 3) & ~3;");
                w.write("if (pos + " + varName + "_padded > end) { console.error('WebCC: OOB " + varName + "_data'); break; }");
                w.write("const " + varName + " = decoder.decode(u8.subarray(pos, pos + " + varName + "_len)); pos += " + varName + "_padded;");
            }
            else
            {
                w.write("// Unknown type: " + p.type);
            }
        }
        w.write(c.action);
        w.write("break;");
        w.write("}");
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
        "webgpu_buffers", "webgpu_pipelines"};

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

    void generate_js_runtime(const SchemaDefs &defs, const std::string &user_code, const std::string &out_dir)
    {
        CodeWriter w;

        std::cout << "[WebCC] Scanning source files for features..." << std::endl;

        std::set<std::string> used_namespaces;
        std::set<std::string> used_maps;
        std::vector<std::string> generated_js_imports;
        CodeWriter cases_w;
        cases_w.set_indent(4);

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
                            ss << "const " << name << " = decoder.decode(new Uint8Array(memory.buffer, " << name << "_ptr, " << name << "_len));\n";
                        }
                    }

                    // Strip outer braces if present to expose local variables (like 'ret')
                    std::string action_body = d.action;
                    size_t open_brace = action_body.find('{');
                    size_t close_brace = action_body.rfind('}');
                    if (open_brace != std::string::npos && close_brace != std::string::npos && close_brace > open_brace)
                    {
                        action_body = action_body.substr(open_brace + 1, close_brace - open_brace - 1);
                    }
                    ss << action_body << "\n";
                    if (d.return_type == "string")
                    {
                        ss << "const encoded = text_encoder.encode(ret);\n";
                        ss << "const len = encoded.length;\n";
                        ss << "new Uint8Array(memory.buffer, scratch_buffer_ptr_val).set(encoded);\n";
                        ss << "return len;\n";
                    }
                    ss << "}";
                    generated_js_imports.push_back(ss.str());
                }
                else
                {
                    // For commands without a return value, generate a case in the flush switch.
                    gen_js_case(d, cases_w);
                }

                used_namespaces.insert(d.ns);
                auto maps = get_maps_from_action(d.action);
                for (const auto &m : maps)
                    used_maps.insert(m);
            }
        }

        w.raw(JS_INIT_HEAD);
        w.set_indent(3);

        for (const auto &imp : generated_js_imports)
        {
            w.raw(",\n");
            w.write(imp);
        }
        w.raw(JS_INIT_TAIL);
        w.set_indent(1);

        // Event System Setup: Set up buffers for JS to send events to C++.
        w.write("const { webcc_event_buffer_ptr, webcc_event_offset_ptr, webcc_event_buffer_capacity, webcc_scratch_buffer_ptr } = mod.instance.exports;");
        w.write("const event_buffer_ptr_val = webcc_event_buffer_ptr();");
        w.write("const event_offset_ptr_val = webcc_event_offset_ptr();");
        w.write("const scratch_buffer_ptr_val = webcc_scratch_buffer_ptr();");
        w.write("let event_offset_view = new Uint32Array(memory.buffer, event_offset_ptr_val, 1);");
        w.write("let event_u8 = new Uint8Array(memory.buffer, event_buffer_ptr_val);");
        w.write("let event_i32 = new Int32Array(memory.buffer, event_buffer_ptr_val);");
        w.write("let event_f32 = new Float32Array(memory.buffer, event_buffer_ptr_val);");
        w.write("const text_encoder = new TextEncoder();");
        w.write("const EVENT_BUFFER_SIZE = webcc_event_buffer_capacity();");
        w.write("");

        // Generate push_event helpers in JS for each event type.
        for (const auto &d : defs.events)
        {
            if (used_namespaces.find(d.ns) == used_namespaces.end())
                continue;

            std::stringstream sig;
            sig << "function push_event_" << d.ns << "_" << d.name << "(";
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                if (i)
                    sig << ", ";
                sig << (d.params[i].name.empty() ? ("arg" + std::to_string(i)) : d.params[i].name);
            }
            sig << ") {";
            w.write(sig.str());

            w.write("if (event_u8.buffer !== memory.buffer) {");
            w.write("event_u8 = new Uint8Array(memory.buffer, event_buffer_ptr_val);");
            w.write("event_i32 = new Int32Array(memory.buffer, event_buffer_ptr_val);");
            w.write("event_f32 = new Float32Array(memory.buffer, event_buffer_ptr_val);");
            w.write("event_offset_view = new Uint32Array(memory.buffer, event_offset_ptr_val, 1);");
            w.write("}");

            w.write("if (event_offset_view[0] + 4096 > EVENT_BUFFER_SIZE) { console.warn('WebCC: Event buffer full, dropping event " + d.name + "'); return; }");
            w.write("let pos = event_offset_view[0];");
            w.write("const start_pos = pos;");
            w.write("event_u8[pos] = " + std::to_string((int)d.opcode) + ";");
            w.write("pos += 4; // Skip header (opcode + pad + size)");

            for (size_t i = 0; i < d.params.size(); ++i)
            {
                const auto &p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                if (p.type == "int32")
                    w.write("event_i32[pos >> 2] = " + name + "; pos += 4;");
                else if (p.type == "uint32")
                    w.write("event_i32[pos >> 2] = " + name + "; pos += 4;");
                else if (p.type == "uint8")
                    w.write("event_i32[pos >> 2] = " + name + "; pos += 4;");
                else if (p.type == "float32")
                    w.write("event_f32[pos >> 2] = " + name + "; pos += 4;");
                else if (p.type == "string")
                {
                    w.write("const encoded_" + std::to_string(i) + " = text_encoder.encode(" + name + ");");
                    w.write("const len_" + std::to_string(i) + " = encoded_" + std::to_string(i) + ".length;");
                    w.write("event_i32[pos >> 2] = len_" + std::to_string(i) + "; pos += 4;");
                    w.write("new Uint8Array(memory.buffer, event_buffer_ptr_val + pos).set(encoded_" + std::to_string(i) + ");");
                    w.write("pos += (len_" + std::to_string(i) + " + 3) & ~3;");
                }
            }
            w.write("const len = pos - start_pos;");
            w.write("event_u8[start_pos + 2] = len & 0xFF;");
            w.write("event_u8[start_pos + 3] = (len >> 8) & 0xFF;");
            w.write("event_offset_view[0] = pos;");
            w.write("}");
        }

        // Emit resource maps (e.g., for DOM elements, canvases) if they are used.
        for (const auto &map : RESOURCE_MAPS)
        {
            if (used_maps.count(map))
            {
                std::string line = "const " + map + " = [];";
                if (map == "elements")
                    line += " elements[0] = document.body;";
                w.write(line);
            }
        }

        w.raw(JS_FLUSH_HEAD);
        w.raw(cases_w.str());
        w.raw(JS_TAIL);
        write_file(out_dir + "/app.js", w.str());
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
        all_sources.push_back(exe_dir + "/src/core/scratch_buffer.cc");
        all_sources.push_back(exe_dir + "/src/core/libc.cc");

        std::string object_files_str;
        bool compilation_failed = false;

        for (const auto &src : all_sources)
        {
            // Generate a unique object file name based on the source path.
            // We replace non-alphanumeric chars with '_' to flatten the path.
            std::string obj_name = src;
            for (char &c : obj_name)
            {
                if (!isalnum(c))
                    c = '_';
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
                          "-o " +
                          out_dir + "/app.wasm " +
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

} // namespace webcc
