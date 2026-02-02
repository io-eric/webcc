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
    static std::string map_cpp_type(const std::string &type, const std::string &name, const std::string &handle_type = "")
    {
        if (type == "string")
            return "webcc::string_view";
        if (type == "handle")
        {
            if (!handle_type.empty())
                return "webcc::" + handle_type;
            return "webcc::handle";
        }
        if ((type == "int32" || type == "uint32") &&
            (name.find("handle") != std::string::npos || name == "id" || name.find("_id") != std::string::npos))
        {
            // Use typed handle if available, otherwise fall back to generic handle
            if (!handle_type.empty())
                return "webcc::" + handle_type;
            return "webcc::handle";
        }
        if (type == "int32")
            return "int32_t";
        if (type == "uint32")
            return "uint32_t";
        if (type == "float32")
            return "float";
        if (type == "float64")
            return "double";
        if (type == "uint8")
            return "uint8_t";
        if (type == "func_ptr")
            return "void*";
        return "void*";
    }

    // Collect all unique handle types from schema
    static std::set<std::string> collect_handle_types(const SchemaDefs &defs)
    {
        std::set<std::string> types;
        for (const auto &c : defs.commands)
        {
            if (!c.return_handle_type.empty())
                types.insert(c.return_handle_type);
            for (const auto &p : c.params)
            {
                if (!p.handle_type.empty())
                    types.insert(p.handle_type);
            }
        }
        for (const auto &e : defs.events)
        {
            for (const auto &p : e.params)
            {
                if (!p.handle_type.empty())
                    types.insert(p.handle_type);
            }
        }
        return types;
    }

    // Emit the handles.h header with all typed handle aliases
    static void emit_handles_header(const std::set<std::string> &handle_types, const std::map<std::string, std::string> &inheritance)
    {
        CodeWriter w;
        w.write("// GENERATED FILE - DO NOT EDIT");
        w.write("#pragma once");
        w.write("#include \"webcc/core/handle.h\"");
        w.write("");
        w.write("namespace webcc {");
        w.write("");
        w.write("// Type-safe handle types auto-generated from schema.def");
        w.write("// Each type is a distinct compile-time type wrapping int32_t");
        w.write("");

        std::set<std::string> emitted;
        std::set<std::string> remaining = handle_types;

        while (!remaining.empty())
        {
            bool progress = false;
            auto it = remaining.begin();
            while (it != remaining.end())
            {
                std::string ht = *it;
                std::string base = "";
                if (inheritance.count(ht))
                    base = inheritance.at(ht);

                // If base exists and not emitted yet
                if (!base.empty() && emitted.find(base) == emitted.end())
                {
                    ++it;
                    continue;
                }

                w.write("// Tag struct for " + ht);
                if (base.empty())
                    w.write("struct " + ht + "_tag {};");
                else
                    w.write("struct " + ht + "_tag : " + base + "_tag {};");

                w.write("using " + ht + " = typed_handle<" + ht + "_tag>;");
                w.write("");

                emitted.insert(ht);
                it = remaining.erase(it);
                progress = true;
            }

            if (!progress)
            {
                // Fallback
                for (const auto &ht : remaining)
                {
                    w.write("// Tag struct for " + ht + " (fallback)");
                    w.write("struct " + ht + "_tag {};");
                    w.write("using " + ht + " = typed_handle<" + ht + "_tag>;");
                    w.write("");
                }
                break;
            }
        }

        w.write("} // namespace webcc");

        write_file("include/webcc/core/handles.h", w.str());
        std::cout << "[WebCC] Emitted include/webcc/core/handles.h with " << handle_types.size() << " typed handles" << std::endl;
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

        // Output handle inheritance map
        w.write("static const std::pair<const char*, const char*> HANDLE_INHERITANCE[] = {");
        for (const auto &kv : defs.handle_inheritance)
        {
            w.write("{ \"" + kv.first + "\", \"" + kv.second + "\" },");
        }
        w.write("{ nullptr, nullptr }");
        w.write("};");
        w.write("");

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
                ss << "{ \"" << d.params[i].type << "\", \"" << name << "\", \"" << d.params[i].handle_type << "\" }";
            }
            ss << "}, ";
            ss << "R\"JS_ACTION(" << d.action << ")JS_ACTION\", ";
            ss << "\"" << d.return_type << "\", \"" << d.return_handle_type << "\" },";
            w.write(ss.str());
        }
        w.write("{ \"\", \"\", 0, \"\", {}, \"\", \"\", \"\" }");
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
                ss << "{ \"" << d.params[i].type << "\", \"" << name << "\", \"" << d.params[i].handle_type << "\" }";
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

        // First, collect and emit all typed handles
        auto handle_types = collect_handle_types(defs);

        // Ensure all base types are included
        for (const auto &kv : defs.handle_inheritance)
        {
            if (handle_types.count(kv.first))
            {
                handle_types.insert(kv.second);
            }
        }

        if (!handle_types.empty())
        {
            emit_handles_header(handle_types, defs.handle_inheritance);
        }

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
            if (!handle_types.empty())
            {
                w.write("#include \"webcc/core/handles.h\"");
            }
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
                    w.write("static constexpr uint8_t OPCODE = EVENT_" + d.name + ";");
                    for (const auto &p : d.params)
                    {
                        std::string type = map_cpp_type(p.type, p.name, p.handle_type);
                        std::string name = p.name;
                        w.write(type + " " + name + ";");
                    }

                    w.write("");
                    w.write("static " + struct_name + " parse(const uint8_t* data, uint32_t len) {");
                    w.write(struct_name + " res;");
                    w.write("uint32_t offset = 0;");
                    for (const auto &p : d.params)
                    {
                        std::string cpp_type = map_cpp_type(p.type, p.name, p.handle_type);
                        if (p.type == "int32" || p.type == "handle")
                        {
                            if (cpp_type.find("webcc::") != std::string::npos && cpp_type != "webcc::handle")
                            {
                                // Typed handle
                                w.write("res." + p.name + " = " + cpp_type + "(*(int32_t*)(data + offset)); offset += 4;");
                            }
                            else if (cpp_type == "webcc::handle")
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
                        else if (p.type == "float64")
                        {
                            // Align to 8 bytes based on actual address (matches JS alignment)
                            w.write("{ uintptr_t addr = (uintptr_t)(data + offset); offset = ((addr + 7) & ~7) - (uintptr_t)data; }");
                            w.write("res." + p.name + " = *(double*)(data + offset); offset += 8;");
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
                    std::string ret_handle_type = d.return_handle_type;
                    if (ret_type == "int32")
                        ret_type = "int32_t";
                    else if (ret_type == "uint32")
                        ret_type = "uint32_t";
                    else if (ret_type == "float32")
                        ret_type = "float";
                    else if (ret_type == "float64")
                        ret_type = "double";

                    // Generate extern "C" import
                    std::string c_ret_type = ret_type;
                    if (ret_type == "string")
                        c_ret_type = "uint32_t"; // Returns length
                    if (ret_type == "handle")
                        c_ret_type = "int32_t"; // Handles are int32 at the ABI level

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
                        else if (p.type == "float64")
                            sig << "double " << name;
                        else if (p.type == "uint8")
                            sig << "uint8_t " << name;
                        else if (p.type == "uint32")
                            sig << "uint32_t " << name;
                        else if (p.type == "int32" || p.type == "handle")
                            sig << "int32_t " << name;
                        else
                            sig << "/*unknown*/ void* " << name;
                    }
                    sig << ");";
                    w.write(sig.str());

                    // Generate inline wrapper
                    std::string wrapper_ret_type;
                    bool ret_is_typed_handle = (ret_type == "handle" && !ret_handle_type.empty());
                    bool ret_is_untyped_handle = (ret_type == "handle" && ret_handle_type.empty());
                    bool ret_is_any_handle = ret_is_typed_handle || ret_is_untyped_handle;

                    if (ret_is_typed_handle)
                        wrapper_ret_type = "webcc::" + ret_handle_type;
                    else if (ret_is_untyped_handle)
                        wrapper_ret_type = "webcc::handle";
                    else if (ret_type == "string")
                        wrapper_ret_type = "webcc::string";
                    else
                        wrapper_ret_type = ret_type;

                    std::stringstream wrap;
                    wrap << "inline " << wrapper_ret_type << " " << d.func_name << "(";
                    for (size_t i = 0; i < d.params.size(); ++i)
                    {
                        if (i)
                            wrap << ", ";
                        const auto &p = d.params[i];
                        std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                        wrap << map_cpp_type(p.type, p.name, p.handle_type) << " " << name;
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
                        if (ret_is_any_handle)
                            call << wrapper_ret_type << "(";
                        call << "webcc_" << d.ns << "_" << d.func_name << "(";
                    }

                    for (size_t i = 0; i < d.params.size(); ++i)
                    {
                        if (i)
                            call << ", ";
                        const auto &p = d.params[i];
                        std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                        std::string cpp_type = map_cpp_type(p.type, p.name, p.handle_type);

                        if (cpp_type == "webcc::string_view")
                        {
                            call << name << ".data(), " << name << ".length()";
                        }
                        else if (cpp_type.find("webcc::") != std::string::npos && cpp_type != "webcc::string_view")
                        {
                            // Any handle type (typed or untyped)
                            call << "(int32_t)" << name;
                        }
                        else
                        {
                            call << name;
                        }
                    }
                    call << ")";
                    if (ret_is_any_handle)
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
                        func << map_cpp_type(p.type, p.name, p.handle_type) << " " << name;
                    }
                }
                func << "){";
                w.write(func.str());
                w.write("push_command((uint32_t)OP_" + d.name + ");");
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    std::string cpp_type = map_cpp_type(p.type, p.name, p.handle_type);

                    if (cpp_type == "webcc::string_view")
                        w.write("webcc::CommandBuffer::push_string(" + name + ".data(), " + name + ".length());");
                    else if (cpp_type.find("webcc::") != std::string::npos && cpp_type != "webcc::string_view")
                        // Any handle type (typed or untyped)
                        w.write("push_data<int32_t>((int32_t)" + name + ");");
                    else if (p.type == "uint8")
                        w.write("push_data<uint32_t>((uint32_t)" + name + ");");
                    else if (p.type == "uint32")
                        w.write("push_data<uint32_t>(" + name + ");");
                    else if (p.type == "int32")
                        w.write("push_data<int32_t>(" + name + ");");
                    else if (p.type == "float32")
                        w.write("push_data<float>(" + name + ");");
                    else if (p.type == "float64")
                        w.write("push_data<double>(" + name + ");");
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
            else if (p.type == "float64")
            {
                w.write("if (pos % 8 !== 0) pos += (8 - (pos % 8));");
                w.write("if (pos + 8 > end) { console.error('WebCC: OOB " + varName + "'); break; }");
                w.write("const " + varName + " = f64[pos >> 3]; pos += 8;");
            }
            else if (p.type == "func_ptr")
            {
                w.write("if (pos + 4 > end) { console.error('WebCC: OOB " + varName + "'); break; }");
                w.write("const " + varName + " = i32[pos >> 2]; pos += 4;");
            }
            else if (p.type == "handle")
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

    JsGenResult generate_js_runtime(const SchemaDefs &defs, const std::string &user_code, const std::string &out_dir)
    {
        CodeWriter w;
        JsGenResult result;

        // Always-needed exports
        result.required_exports.insert("memory");
        result.required_exports.insert("main");
        result.required_exports.insert("__indirect_function_table");

        std::cout << "[WebCC] Scanning source files for features..." << std::endl;

        std::set<std::string> used_namespaces;
        std::set<std::string> used_maps;
        std::set<std::string> used_event_listeners; // Track which event types need delegation
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

            // 3. Special check for iostream which uses system::log and system::error implicitly
            if (!used && d.ns == "system" && (d.func_name == "log" || d.func_name == "error"))
            {
                if (user_code.find("#include <iostream>") != std::string::npos ||
                    user_code.find("#include \"webcc/compat/iostream\"") != std::string::npos ||
                    contains_whole_word(user_code, "std::cout") ||
                    contains_whole_word(user_code, "std::cerr"))
                {
                    used = true;
                }
            }

            // 4. Special check for chrono which uses system::get_time and system::get_date_now implicitly
            if (!used && d.ns == "system" && (d.func_name == "get_time" || d.func_name == "get_date_now"))
            {
                if (user_code.find("#include <webcc/compat/chrono>") != std::string::npos ||
                    user_code.find("#include \"webcc/compat/chrono\"") != std::string::npos ||
                    user_code.find("#include \"webcc/core/chrono.h\"") != std::string::npos ||
                    contains_whole_word(user_code, "std::chrono") ||
                    contains_whole_word(user_code, "webcc::chrono"))
                {
                    used = true;
                }
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

                // Track event listener types for delegation
                if (d.func_name == "add_click_listener")
                    used_event_listeners.insert("click");
                else if (d.func_name == "add_input_listener")
                    used_event_listeners.insert("input");
                else if (d.func_name == "add_change_listener")
                    used_event_listeners.insert("change");
                else if (d.func_name == "add_keydown_listener")
                    used_event_listeners.insert("keydown");
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

        // Track event system exports
        result.required_exports.insert("webcc_event_buffer_ptr");
        result.required_exports.insert("webcc_event_offset_ptr");
        result.required_exports.insert("webcc_event_buffer_capacity");
        result.required_exports.insert("webcc_scratch_buffer_ptr");
        result.required_exports.insert("webcc_command_buffer_ptr");

        // Generate single unified exports destructuring
        std::stringstream exports_ss;
        exports_ss << "const { ";
        exports_ss << "memory, main, __indirect_function_table: table";
        exports_ss << ", webcc_event_buffer_ptr, webcc_event_offset_ptr, webcc_event_buffer_capacity, webcc_scratch_buffer_ptr";
        exports_ss << " } = mod.instance.exports;";
        w.write(exports_ss.str());
        w.write("");

        // Event System Setup: Set up buffers for JS to send events to C++.
        w.write("const event_buffer_ptr_val = webcc_event_buffer_ptr();");
        w.write("const event_offset_ptr_val = webcc_event_offset_ptr();");
        w.write("const scratch_buffer_ptr_val = webcc_scratch_buffer_ptr();");
        w.write("let event_offset_view = new Uint32Array(memory.buffer, event_offset_ptr_val, 1);");
        w.write("let event_u8 = new Uint8Array(memory.buffer, event_buffer_ptr_val);");
        w.write("let event_i32 = new Int32Array(memory.buffer, event_buffer_ptr_val);");
        w.write("let event_f32 = new Float32Array(memory.buffer, event_buffer_ptr_val);");
        w.write("let event_f64 = new Float64Array(memory.buffer, event_buffer_ptr_val);");
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
            w.write("event_f64 = new Float64Array(memory.buffer, event_buffer_ptr_val);");
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
                else if (p.type == "handle")
                    w.write("event_i32[pos >> 2] = " + name + "; pos += 4;");
                else if (p.type == "float32")
                    w.write("event_f32[pos >> 2] = " + name + "; pos += 4;");
                else if (p.type == "float64") {
                    w.write("pos = (pos + 7) & ~7;"); // Align to 8 bytes
                    w.write("event_f64[pos >> 3] = " + name + "; pos += 8;");
                }
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

        // Emit global event delegation listeners (more efficient than per-element listeners)
        if (used_event_listeners.count("click"))
        {
            w.write("");
            w.write("// Global click event delegation - walks up DOM tree to find handler");
            w.write("document.body.addEventListener('click', (e) => {");
            w.write("    let el = e.target;");
            w.write("    while (el && el !== document.body) {");
            w.write("        if (el.dataset.c) { push_event_dom_CLICK(parseInt(el.dataset.c)); return; }");
            w.write("        el = el.parentElement;");
            w.write("    }");
            w.write("});");
        }

        w.raw(JS_FLUSH_HEAD);
        w.raw(cases_w.str());
        w.raw(JS_TAIL);
        write_file(out_dir + "/app.js", w.str());
        std::cout << "[WebCC] Generated " << out_dir << "/app.js" << std::endl;

        return result;
    }

    void generate_html(const std::string &out_dir, const std::string &template_path)
    {
        const std::string script_tag = "    <script src=\"app.js\"></script>";
        std::string html;

        // Try to find a custom template
        std::vector<std::string> template_paths;
        if (!template_path.empty())
        {
            template_paths.push_back(template_path);
        }
        template_paths.push_back("index.template.html");
        template_paths.push_back(out_dir + "/index.template.html");

        std::string found_template;
        for (const auto &path : template_paths)
        {
            std::string content = read_file(path);
            if (!content.empty())
            {
                found_template = path;
                html = content;
                break;
            }
        }

        if (!html.empty())
        {
            // Custom template found - inject script tag
            const std::string placeholder = "{{script}}";
            size_t pos = html.find(placeholder);
            if (pos != std::string::npos)
            {
                // Replace placeholder with script tag
                html.replace(pos, placeholder.length(), script_tag);
            }
            else
            {
                // No placeholder - inject before </body>
                pos = html.rfind("</body>");
                if (pos != std::string::npos)
                {
                    html.insert(pos, script_tag + "\n");
                }
                else
                {
                    // No </body> found - append script tag
                    html += "\n" + script_tag + "\n";
                }
            }
            std::cout << "[WebCC] Using template: " << found_template << std::endl;
        }
        else
        {
            // Default template
            html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
</head>
<body>
)" + script_tag + R"(
</body>
</html>
)";
        }

        write_file(out_dir + "/index.html", html);
        std::cout << "[WebCC] Generated " << out_dir << "/index.html" << std::endl;
    }

    bool compile_wasm(const std::vector<std::string> &input_files, const std::string &out_dir, const std::string &cache_dir, const std::set<std::string> &required_exports)
    {
        // Check Clang version (requires 16+ for full C++20 support)
        FILE *pipe = popen("clang++ --version 2>&1", "r");
        if (pipe)
        {
            char buffer[256];
            std::string version_output;
            while (fgets(buffer, sizeof(buffer), pipe))
            {
                version_output += buffer;
            }
            pclose(pipe);

#ifdef __APPLE__
            // On macOS, check if using Homebrew LLVM (required for wasm-ld)
            // Apple's clang says "Apple clang" while Homebrew's says "Homebrew clang"
            if (version_output.find("Apple clang") != std::string::npos ||
                version_output.find("Apple LLVM") != std::string::npos)
            {
                std::cerr << "[WebCC] Error: Apple's system clang detected. WebCC requires Homebrew LLVM." << std::endl;
                std::cerr << "  Apple's clang does not include wasm-ld (WebAssembly linker)." << std::endl;
                std::cerr << std::endl;
                std::cerr << "  To fix:" << std::endl;
                std::cerr << "    1. Install Homebrew LLVM: brew install llvm lld" << std::endl;
                std::cerr << "    2. Add to PATH (add to ~/.zshrc to make permanent):" << std::endl;
                std::cerr << "       export PATH=\"$(brew --prefix llvm)/bin:$PATH\"" << std::endl;
                std::cerr << "    3. IMPORTANT - Clean old build files (mixing compilers causes errors):" << std::endl;
                std::cerr << "       rm -rf build/ .cache/" << std::endl;
                std::cerr << "    4. Rebuild from scratch" << std::endl;
                std::cerr << std::endl;
                std::cerr << "  Verify with: clang++ --version (should show 'Homebrew clang')" << std::endl;
                return false;
            }
#endif

            // Extract major version number
            size_t pos = version_output.find("clang version ");
            if (pos != std::string::npos)
            {
                int major_version = std::atoi(version_output.c_str() + pos + 14);
                if (major_version > 0 && major_version < 16)
                {
                    std::cerr << "[WebCC] Error: Clang " << major_version << " detected. WebCC requires Clang 16+ for full C++20 support." << std::endl;
                    std::cerr << "  Ubuntu/Debian: sudo apt install clang-16" << std::endl;
                    std::cerr << "  macOS: brew install llvm" << std::endl;
                    return false;
                }
            }
        }

        std::cout << "[WebCC] Compiling..." << std::endl;

        // Ensure cache directory exists
        mkdir(cache_dir.c_str(), 0755);

        std::string exe_dir = get_executable_dir();
        std::vector<std::string> all_sources = input_files;

        // Internal sources
        all_sources.push_back(exe_dir + "/src/core/command_buffer.cc");
        all_sources.push_back(exe_dir + "/src/core/event_buffer.cc");
        all_sources.push_back(exe_dir + "/src/core/scratch_buffer.cc");
        all_sources.push_back(exe_dir + "/src/core/libc.cc");

        // --- 1. CONFIGURATION ---
        // base_cmd: Shared core settings for both compilation and linking.
        std::string base_cmd = "clang++ --target=wasm32 "
                               "-Oz "   // Size optimization
                               "-flto " // Link-time optimization
                               "-std=c++20 "
                               "-nostdlib "
                               "-mbulk-memory "     // Enable bulk memory operations
                               "-mmutable-globals " // Faster global variable access
                               "-msign-ext ";       // Optimize sign extensions

        // include_flags: Tells the compiler where to find headers.
        std::string include_flags = "-isystem \"" + exe_dir + "/include/webcc/compat\" " +
                                    "-I \"" + exe_dir + "/include\" ";

        // compile_only_flags: Settings applied only when generating .o files.
        std::string compile_only_flags = "-fvisibility=hidden "
                                         "-fno-exceptions "
                                         "-fno-rtti "
                                         "-ffunction-sections " // For better dead code elimination
                                         "-fdata-sections "     // For better dead code elimination
                                         "-c ";

        // link_only_flags: Build dynamically from required_exports with MEMORY OPTIMIZATIONS
        std::string link_only_flags = "-Wl,--no-entry ";

        // Export your required functions
        for (const auto &exp : required_exports)
        {
            link_only_flags += "-Wl,--export=" + exp + " ";
        }

        // === CRITICAL MEMORY OPTIMIZATIONS FOR FAST MOUNT ===
        link_only_flags +=
            "-Wl,--gc-sections "     // Remove unused code sections
            "-Wl,--allow-undefined " // Allow undefined symbols (for JS imports)

            // === MEMORY LAYOUT OPTIMIZATIONS ===
            "-Wl,--stack-first "   // Stack at address 0 (grows DOWNWARD)
            "-z stack-size=65536 " // 64KB stack (efficient for UI recursion)

            // Memory allocation (Bumped to 4MB to accommodate static data > 2.1MB)
            "-Wl,--initial-memory=4194304 " // 4MB total
            "-Wl,--max-memory=67108864 "    // 64MB max

            // === PERFORMANCE OPTIMIZATIONS ===
            "-Wl,--compress-relocations " // Smaller binary = faster download
                                          // "-Wl,--lto-O3 "              // Removed: can increase size. Rely on -Oz.

            // === SIZE OPTIMIZATIONS ===
            "-Wl,--strip-debug " // Remove debug info
            "-Wl,--strip-all "   // Strip all symbols
            ;
        // --- 2. COMPILATION LOOP ---
        std::string object_files_str;
        bool compilation_failed = false;

        for (const auto &src : all_sources)
        {
            std::string obj_name = src;
            for (char &c : obj_name)
                if (!isalnum(c))
                    c = '_';
            std::string obj = cache_dir + "/" + obj_name + ".o";

            struct stat src_stat, obj_stat;
            bool need_compile = true;

            if (stat(src.c_str(), &src_stat) == 0)
            {
                if (stat(obj.c_str(), &obj_stat) == 0)
                {
                    if (obj_stat.st_mtime >= src_stat.st_mtime)
                        need_compile = false;
                }
            }
            else
            {
                std::cerr << "[WebCC] Error: Source not found: " << src << std::endl;
                return false;
            }

            if (need_compile)
            {
                std::cout << "  [CC] " << src << std::endl;
                std::string cc_full_cmd = base_cmd + compile_only_flags + include_flags + "-o \"" + obj + "\" \"" + src + "\"";

                if (system(cc_full_cmd.c_str()) != 0)
                {
                    compilation_failed = true;
                    break;
                }
            }
            else
            {
                std::cout << "  [Cache] " << src << std::endl;
            }
            object_files_str += "\"" + obj + "\" ";
        }

        if (compilation_failed)
            return false;

        // --- 3. LINKING ---
        // Check for wasm-ld before attempting to link
        if (system("command -v wasm-ld > /dev/null 2>&1") != 0)
        {
            std::cerr << "[WebCC] Error: wasm-ld not found. Required for WebAssembly linking." << std::endl;
#ifdef __APPLE__
            std::cerr << "  Install: brew install llvm lld" << std::endl;
            std::cerr << "  Then add to PATH: export PATH=\"$(brew --prefix llvm)/bin:$PATH\"" << std::endl;
#else
            std::cerr << "  Ubuntu/Debian: sudo apt install lld" << std::endl;
            std::cerr << "  Fedora: sudo dnf install lld" << std::endl;
            std::cerr << "  Arch Linux: sudo pacman -S lld" << std::endl;
#endif
            return false;
        }

        std::cout << "[WebCC] Linking..." << std::endl;
        std::string wasm_path = out_dir + "/app.wasm";
        std::string link_full_cmd = base_cmd + link_only_flags + "-o \"" + wasm_path + "\" " + object_files_str;

        if (system(link_full_cmd.c_str()) != 0)
        {
            std::cerr << "[WebCC] Linking failed!" << std::endl;
            return false;
        }

        // --- 4. POST-OPTIMIZATION ---
        /*
        if (system("command -v wasm-opt > /dev/null") == 0)
        {
            std::cout << "[WebCC] Optimizing with wasm-opt..." << std::endl;
            std::string opt_cmd = "wasm-opt -Oz --strip-debug " + wasm_path + " -o " + wasm_path;
            system(opt_cmd.c_str());
        }
        */
        std::cout << "[WebCC] Success! Run 'python3 -m http.server' in " << out_dir << " to view." << std::endl;
        return true;
    }
} // namespace webcc
