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
}
