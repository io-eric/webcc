#include "generators.h"
#include "utils.h"
#include "js_templates.h"
#include "scratch_buffer.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <cctype>
#include <cstdio>
#include <set>
#include <map>
#include <regex>
#include <sys/stat.h>

namespace webcc
{

    // Helper to map schema types to C++ types
    static std::string map_cpp_type(const std::string &type, const std::string &name, const std::string &handle_type = "")
    {
        if (type == "string")
            return "webcc::string_view";
        if (type == "bytes")
            return "webcc::bytes_view";
        if (type == "handle")
        {
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
        for (const auto &k : defs.consts)
            namespaces.insert(k.ns);

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
            w.write("#include \"webcc/core/bytes_view.h\"");
            w.write("namespace webcc::" + ns + " {");

            // Named constants
            bool has_consts = false;
            for (const auto &k : defs.consts)
            {
                if (k.ns != ns)
                    continue;
                w.write("inline constexpr int32_t " + k.name + " = " + k.value + ";");
                has_consts = true;
            }
            if (has_consts)
                w.write("");

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
                        else if (p.type == "bytes")
                        {
                            w.write("uint32_t " + p.name + "_len = *(uint32_t*)(data + offset); offset += 4;");
                            w.write("res." + p.name + " = webcc::bytes_view(data + offset, " + p.name + "_len);");
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
                    else if (ret_type == "uint8")
                        ret_type = "uint8_t";
                    else if (ret_type == "float32")
                        ret_type = "float";
                    else if (ret_type == "float64")
                        ret_type = "double";

                    // Generate extern "C" import
                    std::string c_ret_type = ret_type;
                    if (ret_type == "string" || ret_type == "bytes")
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
                        else if (p.type == "bytes")
                            sig << "const uint8_t* " << name << ", uint32_t " << name << "_len";
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
                    else if (ret_type == "bytes")
                        wrapper_ret_type = "webcc::vector<uint8_t>";
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
                        if (!p.default_value.empty())
                            wrap << " = " << p.default_value;
                    }
                    wrap << "){";
                    w.write(wrap.str());
                    w.write("::webcc::flush();");

                    std::stringstream call;
                    if (ret_type == "string" || ret_type == "bytes")
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

                        if (cpp_type == "webcc::string_view" || cpp_type == "webcc::bytes_view")
                        {
                            call << name << ".data(), " << name << ".length()";
                        }
                        else if (cpp_type.find("webcc::") != std::string::npos)
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
                        w.write("return ::webcc::take_string_result(len);");
                    else if (ret_type == "bytes")
                        w.write("return ::webcc::take_bytes_result(len);");

                    w.write("}");
                    w.write("");
                    continue;
                }

                // Per-command feature marker: an imported function (module "w",
                // field = opcode) whose address is parked in a `used` static
                // inside the wrapper below. It is never called -- so it costs
                // nothing at runtime -- but it appears in the linked module's
                // import table iff this wrapper is actually referenced by user
                // code. That import is how generate_js_runtime detects, exactly
                // and without scanning source text, that this void command is
                // used (return-value commands are caught via their real import).
                std::string mark_op = std::to_string((int)d.opcode);
                w.write("extern \"C\" __attribute__((import_module(\"w\"), import_name(\"" + mark_op + "\"))) void __webcc_m_" + mark_op + "(void);");

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
                        if (!p.default_value.empty())
                            func << " = " << p.default_value;
                    }
                }
                func << "){";
                w.write(func.str());
                w.write("[[maybe_unused]] static void (*const __webcc_keep)(void) __attribute__((used)) = &__webcc_m_" + mark_op + ";");
                w.write("push_command((uint32_t)OP_" + d.name + ");");
                for (size_t i = 0; i < d.params.size(); ++i)
                {
                    const auto &p = d.params[i];
                    std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                    std::string cpp_type = map_cpp_type(p.type, p.name, p.handle_type);

                    if (cpp_type == "webcc::string_view")
                        w.write("webcc::CommandBuffer::push_string(" + name + ".data(), " + name + ".length());");
                    else if (cpp_type == "webcc::bytes_view")
                        w.write("webcc::CommandBuffer::push_string((const char*)" + name + ".data(), " + name + ".length());");
                    else if (cpp_type.find("webcc::") != std::string::npos)
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
        
        // Save binary cache for fast runtime loading (no need to recompile webcc)
        save_defs_binary(defs, "schema.wcc.bin");
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
            else if (p.type == "bytes")
            {
                // View into wasm memory, only valid during the action
                w.write("if (pos + 4 > end) { console.error('WebCC: OOB " + varName + "_len'); break; }");
                w.write("const " + varName + "_len = i32[pos >> 2]; pos += 4;");
                w.write("const " + varName + "_padded = (" + varName + "_len + 3) & ~3;");
                w.write("if (pos + " + varName + "_padded > end) { console.error('WebCC: OOB " + varName + "_data'); break; }");
                w.write("const " + varName + " = u8.subarray(pos, pos + " + varName + "_len); pos += " + varName + "_padded;");
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
        "elements", "contexts", "audios", "websockets", "images", "blobs", "databases", "fetches",
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

    std::set<std::string> get_pushed_events_from_action(const std::string &action)
    {
        std::set<std::string> events;
        static const std::regex push_event_regex(R"(push_event_([a-zA-Z0-9]+)_([A-Z0-9_]+)\s*\()");

        auto begin = std::sregex_iterator(action.begin(), action.end(), push_event_regex);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it)
        {
            std::string ns = (*it)[1].str();
            std::string name = (*it)[2].str();
            events.insert(ns + "::" + name);
        }

        return events;
    }

    const std::set<std::string> &required_wasm_exports()
    {
        // Symbols the JS runtime always reads from the instantiated module.
        // This set is constant and independent of which features are used, so
        // the wasm can be linked *before* feature detection runs -- which is
        // what lets us read the import table to discover used commands.
        static const std::set<std::string> exports = {
            "memory",
            "main",
            "__indirect_function_table",
            "webcc_event_buffer_ptr",
            "webcc_event_offset_ptr",
            "webcc_event_buffer_capacity",
            "webcc_scratch_buffer_ptr",
            "webcc_command_buffer_ptr",
        };
        return exports;
    }

    // Escape a raw byte string into a JS string literal (including the quotes).
    // Used to turn a WEBCC_JS snippet's source -- which is also its wasm import
    // name -- into a valid object key that parses back to the identical bytes.
    static std::string js_quote(const std::string &s)
    {
        std::string out = "\"";
        for (unsigned char c : s)
        {
            switch (c)
            {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20)
                {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                }
                else
                    out += (char)c;
            }
        }
        out += "\"";
        return out;
    }

    // A parsed WEBCC_JS parameter: the identifier plus how the JS handler
    // should adapt the incoming wasm value.
    struct JsFnParam
    {
        std::string name;
        bool is_string;   // const char*: decode the pointer to a JS string
        bool is_unsigned; // unsigned/uint*: reinterpret the i32 as unsigned
    };

    // Split a WEBCC_JS import name -- "name(params){body}" -- into its parts.
    // The shape is guaranteed by the macro: an identifier, a balanced paren
    // group, then the brace body.
    static bool parse_js_fn(const std::string &imp, std::string &name,
                            std::string &params_raw, std::string &body)
    {
        size_t i = 0;
        while (i < imp.size() && (std::isalnum((unsigned char)imp[i]) || imp[i] == '_'))
            ++i;
        if (i == 0 || i >= imp.size() || imp[i] != '(')
            return false;
        name = imp.substr(0, i);

        int depth = 0;
        size_t j = i;
        for (; j < imp.size(); ++j)
        {
            if (imp[j] == '(')
                ++depth;
            else if (imp[j] == ')' && --depth == 0)
                break;
        }
        if (j >= imp.size())
            return false;
        params_raw = imp.substr(i + 1, j - i - 1); // inside the outer parens
        body = imp.substr(j + 1);                  // the "{ ... }" remainder
        return true;
    }

    // Extract one entry per declared parameter (name + marshalling hints). The
    // name is the trailing identifier of each comma-separated declaration; the
    // type prefix decides string/unsigned handling. Unnamed params and `void`
    // are skipped.
    static std::vector<JsFnParam> parse_js_fn_params(const std::string &params_raw)
    {
        std::vector<JsFnParam> out;
        std::vector<std::string> parts;
        int depth = 0;
        std::string cur;
        for (char c : params_raw)
        {
            if (c == '(' || c == '<' || c == '[')
                ++depth;
            else if (c == ')' || c == '>' || c == ']')
                --depth;
            if (c == ',' && depth == 0)
            {
                parts.push_back(cur);
                cur.clear();
            }
            else
                cur += c;
        }
        if (!cur.empty())
            parts.push_back(cur);

        for (auto &p : parts)
        {
            size_t a = p.find_first_not_of(" \t");
            size_t b = p.find_last_not_of(" \t");
            if (a == std::string::npos)
                continue;
            std::string t = p.substr(a, b - a + 1);
            if (t == "void")
                continue;
            size_t e = t.size();
            while (e > 0 && (std::isalnum((unsigned char)t[e - 1]) || t[e - 1] == '_'))
                --e;
            std::string pname = t.substr(e);
            if (pname.empty())
                continue; // unnamed parameter: nothing to bind in JS
            std::string type = t.substr(0, e);
            bool is_string = type.find("char") != std::string::npos && type.find('*') != std::string::npos;
            bool is_unsigned = type.find("unsigned") != std::string::npos || type.find("uint") != std::string::npos;
            out.push_back({pname, is_string, is_unsigned});
        }
        return out;
    }

    // Emit the "wjs_fn" import module: one handler per named WEBCC_JS. The
    // import name carries the full `name(params){body}` source; we reproduce it
    // as `(names) => { <marshalling> <body> }`. Sets need_utf8 if any handler
    // needs the NUL-terminated string decoder.
    static void emit_inline_js_fn_module(CodeWriter &w, const std::set<std::string> &fns, bool &need_utf8)
    {
        if (fns.empty())
            return;
        w.raw(",\n        wjs_fn: {");
        bool first = true;
        for (const auto &imp : fns)
        {
            std::string name, params_raw, body;
            if (!parse_js_fn(imp, name, params_raw, body))
                continue;
            auto params = parse_js_fn_params(params_raw);

            // Strip the body's outer braces so we can wrap it in the handler.
            std::string inner = body;
            size_t ob = inner.find('{'), cb = inner.rfind('}');
            if (ob != std::string::npos && cb != std::string::npos && cb > ob)
                inner = inner.substr(ob + 1, cb - ob - 1);

            std::string namelist, prelude;
            for (size_t k = 0; k < params.size(); ++k)
            {
                if (k)
                    namelist += ", ";
                namelist += params[k].name;
                if (params[k].is_string)
                {
                    prelude += params[k].name + " = __webcc_utf8(" + params[k].name + "); ";
                    need_utf8 = true;
                }
                else if (params[k].is_unsigned)
                    prelude += params[k].name + " = " + params[k].name + " >>> 0; ";
            }

            w.raw(first ? "\n" : ",\n");
            first = false;
            w.raw("            " + js_quote(imp) + ": (" + namelist + ") => {" + prelude + inner + "}");
        }
        w.raw("\n        }");
    }

    void generate_js_runtime(const SchemaDefs &defs, const std::set<std::string> &wasm_imports, const std::set<std::string> &void_markers, const std::set<std::string> &inline_js_fns, const std::string &out_dir)
    {
        CodeWriter w;

        std::cout << "[WebCC] Detecting features..." << std::endl;

        std::set<std::string> used_namespaces;
        std::set<std::string> used_maps;
        std::vector<const std::string *> used_actions; // for helper detection
        std::set<std::string> used_event_listeners; // Track which event types need delegation
        std::set<std::string> used_event_helpers;   // Track which push_event helpers must exist
        std::vector<std::string> generated_js_imports;
        bool any_void_command_used = false; // whether any void command (and thus marker import) is used
        bool any_buffered_return = false;   // whether any string/bytes-returning command is used
        CodeWriter cases_w;
        cases_w.set_indent(4);

        // Detect which commands are used entirely from the linked module's
        // import table -- no source scanning. Both kinds of command leave an
        // import when (and only when) the user's code references them:
        //   * Return-value commands are real wasm imports (webcc_<ns>_<func>),
        //     called directly from the wrapper.
        //   * Void commands are batched and never call across the boundary, so
        //     each wrapper instead parks the address of a per-command marker
        //     import (module "w", field = opcode) in a `used` static. The
        //     marker is never called, but it appears in the import table iff
        //     the wrapper is live -- which `void_markers` reflects exactly.
        // This is immune to aliases, macros, and compat-header lowering (e.g.
        // std::cout -> system::log) that a text scan would silently miss.

        for (const auto &d : defs.commands)
        {
            bool used;
            if (!d.return_type.empty())
                used = wasm_imports.count("webcc_" + d.ns + "_" + d.func_name) > 0;
            else
                used = void_markers.count(std::to_string((int)d.opcode)) > 0;

            if (!used)
                continue;

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
                        if (p.type == "string" || p.type == "bytes")
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
                        else if (p.type == "bytes")
                        {
                            // View into wasm memory, only valid during the call
                            ss << "const " << name << " = new Uint8Array(memory.buffer, " << name << "_ptr, " << name << "_len);\n";
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
                        // Small results go through the scratch buffer, larger ones
                        // wait in JS until C++ fetches them (webcc_js_read_result)
                        ss << "const encoded = text_encoder.encode(ret);\n";
                        ss << "if (encoded.length > " << SCRATCH_BUFFER_SIZE << ") _big_result = encoded;\n";
                        ss << "else new Uint8Array(memory.buffer, scratch_buffer_ptr_val).set(encoded);\n";
                        ss << "return encoded.length;\n";
                        any_buffered_return = true;
                    }
                    else if (d.return_type == "bytes")
                    {
                        // Same path as strings, `ret` is a Uint8Array
                        ss << "if (ret.length > " << SCRATCH_BUFFER_SIZE << ") _big_result = ret;\n";
                        ss << "else new Uint8Array(memory.buffer, scratch_buffer_ptr_val).set(ret);\n";
                        ss << "return ret.length;\n";
                        any_buffered_return = true;
                    }
                    ss << "}";
                    generated_js_imports.push_back(ss.str());
                }
                else
                {
                    // For commands without a return value, generate a case in the flush switch.
                    // (Dispatched by opcode -- no JS import needed.) Its marker
                    // import still needs a no-op stub at instantiation time.
                    gen_js_case(d, cases_w);
                    any_void_command_used = true;
                }

                used_namespaces.insert(d.ns);
                used_actions.push_back(&d.action);
                auto maps = get_maps_from_action(d.action);
                for (const auto &m : maps)
                    used_maps.insert(m);

                auto pushed_events = get_pushed_events_from_action(d.action);
                for (const auto &e : pushed_events)
                    used_event_helpers.insert(e);

                // Track event listener types for delegation
                if (d.func_name == "add_click_listener")
                    used_event_listeners.insert("click");
                else if (d.func_name == "add_input_listener")
                    used_event_listeners.insert("input");
                else if (d.func_name == "add_change_listener")
                    used_event_listeners.insert("change");
                else if (d.func_name == "add_keydown_listener")
                    used_event_listeners.insert("keydown");
                else if (d.func_name == "init_mouse") {
                    used_event_listeners.insert("mouse_down");
                    used_event_listeners.insert("mouse_up");
                    used_event_listeners.insert("mouse_move");
                }
                else if (d.func_name == "init_keyboard") {
                    used_event_listeners.insert("key_down");
                    used_event_listeners.insert("key_up");
                }
            }
        }

        if (any_buffered_return)
            generated_js_imports.push_back("webcc_js_read_result: (ptr) => { new Uint8Array(memory.buffer, ptr, _big_result.length).set(_big_result); _big_result = null; }");

        w.raw(JS_INIT_HEAD);
        w.set_indent(3);

        for (const auto &imp : generated_js_imports)
        {
            w.raw(",\n");
            w.write(imp);
        }
        w.raw(JS_INIT_ENV_CLOSE);

        // Sibling "w" import module: every used void command leaves a marker
        // import (see emit_headers) that must be supplied at instantiation even
        // though it is never called. A Proxy returns the shared no-op for any
        // marker name, so this stays one line no matter how many are used.
        if (any_void_command_used)
            w.raw(",\n        w: new Proxy({}, { get: () => _wm })");

        // Inline-JS escape hatch (WEBCC_JS): one sibling "wjs_fn" handler per
        // named function the link actually retained.
        bool need_js_utf8 = false;
        emit_inline_js_fn_module(w, inline_js_fns, need_js_utf8);

        w.raw(JS_INIT_INSTANTIATE);
        w.set_indent(1);

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
        if (any_buffered_return)
            w.write("let _big_result = null;");

        // Decoder for `const char*` parameters of named WEBCC_JS functions:
        // read the NUL-terminated UTF-8 string at `ptr` from linear memory.
        if (need_js_utf8)
        {
            w.write("const __webcc_utf8 = (ptr) => {");
            w.write("    const m = new Uint8Array(memory.buffer);");
            w.write("    let e = ptr; while (m[e] !== 0) e++;");
            w.write("    return decoder.decode(m.subarray(ptr, e));");
            w.write("};");
        }
        w.write("const EVENT_BUFFER_SIZE = webcc_event_buffer_capacity();");
        w.write("");
        w.write("// Global update function reference for immediate discrete event processing");
        w.write("let _updateFn = null;");
        w.write("let _updatePending = false;");
        w.write("// set_update: no rAF loop, frames run only after an event or request_frame()");
        w.write("let _frameOnDemand = false;");
        w.write("let _frameRaf = 0;");
        w.write("function _requestFrame() {");
        w.write("    if (_frameRaf || (_updateFn && !_frameOnDemand)) return;");
        w.write("    _frameRaf = requestAnimationFrame((t) => { _frameRaf = 0; if (_updateFn && _frameOnDemand) _updateFn(t); });");
        w.write("}");
        w.write("function _triggerDiscreteUpdate() {");
        w.write("    if (_updateFn && !_updatePending) {");
        w.write("        _updatePending = true;");
        w.write("        queueMicrotask(() => {");
        w.write("            _updatePending = false;");
        w.write("            // This run is the frame for the events so far; the update can ask for another");
        w.write("            if (_frameRaf) { cancelAnimationFrame(_frameRaf); _frameRaf = 0; }");
        w.write("            _updateFn(performance.now());");
        w.write("        });");
        w.write("    }");
        w.write("}");
        w.write("");

        // JS helpers a used action (or another used helper) mentions; their own map and
        // event usage counts like an action's
        std::vector<const SchemaHelper *> used_helpers;
        {
            std::vector<const std::string *> scan = used_actions;
            for (size_t i = 0; i < scan.size(); i++)
                for (const auto &h : defs.helpers)
                {
                    if (std::find(used_helpers.begin(), used_helpers.end(), &h) != used_helpers.end()) continue;
                    if (!contains_whole_word(*scan[i], h.name)) continue;
                    used_helpers.push_back(&h);
                    scan.push_back(&h.code);
                    for (const auto &m : get_maps_from_action(h.code)) used_maps.insert(m);
                    for (const auto &e : get_pushed_events_from_action(h.code)) used_event_helpers.insert(e);
                }
        }

        // Generate push_event helpers in JS only for event types that are actually used.
        for (const auto &d : defs.events)
        {
            bool helper_needed = used_event_helpers.count(d.ns + "::" + d.name) > 0;

            // Convert event name to lowercase for matching against used_event_listeners
            std::string event_lower = d.name;
            for (auto &c : event_lower) c = std::tolower(c);

            if (!helper_needed && used_event_listeners.find(event_lower) == used_event_listeners.end())
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

            // Worst-case size of the event, so it is only written if it fits
            uint32_t fixed_size = 4; // header
            std::string dynamic_size;
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                const auto &p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                std::string idx = std::to_string(i);
                if (p.type == "string")
                {
                    w.write("const data_" + idx + " = text_encoder.encode(" + name + ");");
                    fixed_size += 8; // length + padding
                    dynamic_size += " + data_" + idx + ".length";
                }
                else if (p.type == "bytes")
                {
                    w.write("const data_" + idx + " = " + name + ";");
                    fixed_size += 8;
                    dynamic_size += " + data_" + idx + ".length";
                }
                else if (p.type == "float64")
                    fixed_size += 12; // value + alignment
                else
                    fixed_size += 4;
            }

            w.write("let pos = event_offset_view[0];");
            w.write("if (pos + " + std::to_string(fixed_size) + dynamic_size + " > EVENT_BUFFER_SIZE) { console.warn('WebCC: Event buffer full, dropping event " + d.name + "'); return; }");
            w.write("const start_pos = pos;");
            w.write("pos += 4; // Skip header (opcode + size)");

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
                else if (p.type == "string" || p.type == "bytes")
                {
                    std::string data = "data_" + std::to_string(i);
                    w.write("event_i32[pos >> 2] = " + data + ".length; pos += 4;");
                    w.write("event_u8.set(" + data + ", pos);");
                    w.write("pos += (" + data + ".length + 3) & ~3;");
                }
            }
            // Header: [Opcode:1][SizeHi:1][SizeLo:2]
            w.write("const len = pos - start_pos;");
            w.write("event_i32[start_pos >> 2] = " + std::to_string((int)d.opcode) + " | (len >> 16 << 8) | (len << 16);");
            w.write("event_offset_view[0] = pos;");
            w.write("_requestFrame();");
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

        for (const auto *h : used_helpers)
        {
            w.write("");
            std::istringstream code(h->code);
            std::string line;
            while (std::getline(code, line))
                w.write(line);
        }

        // Emit global event delegation listeners (more efficient than per-element listeners)
        if (used_event_listeners.count("click"))
        {
            w.write("");
            w.write("// Global click delegation with bubbling semantics (target -> ancestors)");
            w.write("document.body.addEventListener('click', (e) => {");
            w.write("    let el = e.target;");
            w.write("    let pushed = false;");
            w.write("    while (el && el !== document.body) {");
            w.write("        if (el.dataset.c) { push_event_dom_CLICK(parseInt(el.dataset.c)); pushed = true; }");
            w.write("        el = el.parentElement;");
            w.write("    }");
            w.write("    if (pushed) _triggerDiscreteUpdate();");
            w.write("});");
        }

        if (used_event_listeners.count("input"))
        {
            w.write("");
            w.write("// Global input delegation with bubbling semantics (target -> ancestors)");
            w.write("document.body.addEventListener('input', (e) => {");
            w.write("    let el = e.target;");
            w.write("    let pushed = false;");
            w.write("    while (el && el !== document.body) {");
            w.write("        if (el.dataset.i) { push_event_dom_INPUT(parseInt(el.dataset.i), e.target.value || ''); pushed = true; }");
            w.write("        el = el.parentElement;");
            w.write("    }");
            w.write("    if (pushed) _triggerDiscreteUpdate();");
            w.write("});");
        }

        if (used_event_listeners.count("change"))
        {
            w.write("");
            w.write("// Global change delegation with bubbling semantics (target -> ancestors)");
            w.write("document.body.addEventListener('change', (e) => {");
            w.write("    let el = e.target;");
            w.write("    let pushed = false;");
            w.write("    while (el && el !== document.body) {");
            w.write("        if (el.dataset.g) {");
            w.write("            const val = e.target.type === 'checkbox' ? (e.target.checked ? 'true' : 'false') : (e.target.value || '');");
            w.write("            push_event_dom_CHANGE(parseInt(el.dataset.g), val); pushed = true;");
            w.write("        }");
            w.write("        el = el.parentElement;");
            w.write("    }");
            w.write("    if (pushed) _triggerDiscreteUpdate();");
            w.write("});");
        }

        if (used_event_listeners.count("keydown"))
        {
            w.write("");
            w.write("// Global keydown delegation with bubbling semantics (target -> ancestors)");
            w.write("document.body.addEventListener('keydown', (e) => {");
            w.write("    let el = e.target;");
            w.write("    let pushed = false;");
            w.write("    while (el && el !== document.body) {");
            w.write("        if (el.dataset.k) { push_event_dom_KEYDOWN(parseInt(el.dataset.k), e.keyCode); pushed = true; }");
            w.write("        el = el.parentElement;");
            w.write("    }");
            w.write("    if (pushed) _triggerDiscreteUpdate();");
            w.write("});");
        }

        w.raw(JS_FLUSH_HEAD);
        w.raw(cases_w.str());
        w.raw(JS_TAIL);
        write_file(out_dir + "/app.js", w.str());
        std::cout << "[WebCC] Generated " << out_dir << "/app.js" << std::endl;
    }

    void generate_html(const std::string &out_dir, const std::string &template_path)
    {
        const std::string script_tag = "    <script src=\"./app.js\"></script>";
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

        // Toolchain-version guard for the object cache. The cache keys only on
        // source mtime, so a rebuilt webcc -- or headers regenerated in the same
        // build step -- would otherwise leave stale objects in place. That now
        // matters for correctness, not just freshness: feature detection reads
        // per-command markers out of the compiled object, so a stale object
        // compiled against old headers silently under-detects commands. Treat
        // the webcc binary's mtime as the toolchain version and recompile any
        // object older than it. (Only triggers right after a toolchain rebuild;
        // ordinary app edits leave the binary's mtime untouched.)
        // A schema change regenerates the headers and schema.wcc.bin (next to the
        // binary) without relinking webcc, and renumbers opcodes, so it counts too.
        struct stat exe_stat;
        bool have_exe_mtime = (stat(get_executable_path().c_str(), &exe_stat) == 0);
        if (have_exe_mtime)
        {
            std::string exe_path = get_executable_path();
            std::string schema_bin = exe_path.substr(0, exe_path.find_last_of('/') + 1) + "schema.wcc.bin";
            struct stat schema_stat;
            if (stat(schema_bin.c_str(), &schema_stat) == 0 && schema_stat.st_mtime > exe_stat.st_mtime)
                exe_stat.st_mtime = schema_stat.st_mtime;
        }

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
                    if (obj_stat.st_mtime >= src_stat.st_mtime &&
                        (!have_exe_mtime || obj_stat.st_mtime >= exe_stat.st_mtime))
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
        return true;
    }
} // namespace webcc
