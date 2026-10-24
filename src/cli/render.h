#pragma once
#include "schema.h"
#include <string>
#include <vector>

namespace webcc
{
    // A page to render: the URL path the program sees, the file the HTML goes to
    struct RenderTarget
    {
        std::string path;
        std::string out_html;
    };

    // Builds the sources for the host with src/core/host_render.cc as the runtime, then runs
    // the program once per target and writes the HTML of its first render. Needs a host clang++.
    bool render_first_frame(const SchemaDefs &defs, const std::vector<std::string> &input_files,
                            const std::string &cache_dir, const std::vector<RenderTarget> &targets);

    // Builds the sources for the host the same way and runs the program once, its output going
    // where ours does. Returns the program's exit code, 1 if it couldn't be built or run.
    int run_on_host(const SchemaDefs &defs, const std::vector<std::string> &input_files, const std::string &cache_dir);
}
