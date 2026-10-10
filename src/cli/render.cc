#include "render.h"
#include "utils.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace webcc
{
    namespace
    {
        // the C import, as the headers declare it (see emit_headers)
        std::string import_signature(const SchemaCommand &d)
        {
            std::string ret = d.return_type;
            if (ret == "int32" || ret == "handle") ret = "int32_t";
            else if (ret == "uint32" || ret == "string" || ret == "bytes") ret = "uint32_t";
            else if (ret == "uint8") ret = "uint8_t";
            else if (ret == "float32") ret = "float";
            else if (ret == "float64") ret = "double";
            std::stringstream sig;
            sig << ret << " webcc_" << d.ns << "_" << d.func_name << "(";
            for (size_t i = 0; i < d.params.size(); ++i)
            {
                if (i) sig << ", ";
                const auto &p = d.params[i];
                std::string name = p.name.empty() ? ("arg" + std::to_string(i)) : p.name;
                if (p.type == "string") sig << "const char* " << name << ", uint32_t " << name << "_len";
                else if (p.type == "bytes") sig << "const uint8_t* " << name << ", uint32_t " << name << "_len";
                else if (p.type == "float32") sig << "float " << name;
                else if (p.type == "float64") sig << "double " << name;
                else if (p.type == "uint8") sig << "uint8_t " << name;
                else if (p.type == "uint32") sig << "uint32_t " << name;
                else sig << "int32_t " << name;
            }
            sig << ")";
            return sig.str();
        }

        // what host_render.cc needs from the schema: how to step over any command
        std::string host_layouts(const SchemaDefs &defs)
        {
            uint16_t max_op = 0;
            for (const auto &d : defs.commands)
                if (d.return_type.empty() && d.opcode > max_op) max_op = d.opcode;
            std::vector<std::string> layouts(max_op + 1);
            std::vector<bool> known(max_op + 1, false);
            for (const auto &d : defs.commands)
            {
                if (!d.return_type.empty()) continue;
                std::string l;
                for (const auto &p : d.params)
                {
                    if (p.type == "string" || p.type == "bytes") l += 's';
                    else if (p.type == "float32") l += 'f';
                    else if (p.type == "float64") l += 'd';
                    else l += 'i';
                }
                layouts[d.opcode] = l;
                known[d.opcode] = true;
            }
            std::stringstream out;
            out << "// generated from the schema for host_render.cc\n";
            out << "#include <cstdint>\n#include <cstddef>\n";
            out << "static const size_t kLayoutCount = " << (max_op + 1) << ";\n";
            out << "static const char* const kLayouts[kLayoutCount] = {\n";
            for (size_t i = 0; i <= max_op; i++)
                out << "    " << (known[i] ? "\"" + layouts[i] + "\"" : "nullptr") << ",\n";
            out << "};\n";
            return out.str();
        }

        // a default answer for every import with a result; weak, host_render.cc has the real DOM ones
        std::string host_stubs(const SchemaDefs &defs)
        {
            std::stringstream out;
            out << "// generated from the schema for host_render.cc\n#include <cstdint>\n";
            for (const auto &d : defs.commands)
            {
                if (d.return_type.empty()) continue;
                out << "extern \"C\" __attribute__((weak)) " << import_signature(d) << " { return 0; }\n";
            }
            out << "// the per-command markers feature detection reads from a wasm build\n";
            for (const auto &d : defs.commands)
            {
                if (!d.return_type.empty()) continue;
                out << "extern \"C\" void __webcc_m_" << d.opcode << "(void) {}\n";
            }
            return out.str();
        }
    }

    bool render_first_frame(const SchemaDefs &defs, const std::vector<std::string> &input_files,
                            const std::string &cache_dir, const std::string &out_html)
    {
        std::string exe_dir = get_executable_dir();
        fs::path host_dir = fs::path(cache_dir) / "host";
        fs::create_directories(host_dir);
        write_file((host_dir / "host_layouts.inc").string(), host_layouts(defs));
        write_file((host_dir / "host_stubs.cc").string(), host_stubs(defs));

        std::string cmd = "clang++ -std=c++20 -O0 -fno-exceptions -fno-rtti -Wno-unknown-attributes "
                          "-I \"" + exe_dir + "/include\" -I \"" + host_dir.string() + "\" ";
        for (const auto &f : input_files) cmd += "\"" + f + "\" ";
        for (const char *core : {"command_buffer.cc", "event_buffer.cc", "scratch_buffer.cc", "host_render.cc"})
            cmd += "\"" + exe_dir + "/src/core/" + core + "\" ";
        cmd += "\"" + (host_dir / "host_stubs.cc").string() + "\" ";
        if (const char *ld = getenv("LDFLAGS_LIBCXX")) cmd += std::string(ld) + " ";
        fs::path bin = host_dir / "render";
        cmd += "-o \"" + bin.string() + "\"";
        if (!quiet) std::cout << "[WebCC] Building for the host" << std::endl;
        report("compiling for the host");
        if (system(cmd.c_str()) != 0)
        {
            std::cerr << "[WebCC] Error: the host build failed" << std::endl;
            return false;
        }

        report("rendering");
        std::string run = "\"" + bin.string() + "\" > \"" + out_html + "\"";
        if (system(run.c_str()) != 0)
        {
            std::cerr << "[WebCC] Error: the page failed while rendering" << std::endl;
            return false;
        }
        if (!quiet) std::cout << "[WebCC] Rendered " << out_html << std::endl;
        return true;
    }
}
