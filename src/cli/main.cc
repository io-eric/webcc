#include "utils.h"
#include "schema.h"
#include "js_templates.h"
#include "generators.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <set>
#include <vector>

#if __has_include("webcc_schema.h")
#include "webcc_schema.h"
#define WEBCC_HAS_SCHEMA 1
#else
#define WEBCC_HAS_SCHEMA 0
#endif

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
    generate_js_runtime(defs, user_code, out_dir);

    // C. GENERATE HTML (Basic scaffolding)
    generate_html(out_dir);

    // D. COMPILE C++ TO WASM (Incremental)
    if (!compile_wasm(input_files, out_dir, build_dir))
    {
        return 1;
    }

    return 0;
}
