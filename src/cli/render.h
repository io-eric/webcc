#pragma once
#include "schema.h"
#include <string>
#include <vector>

namespace webcc
{
    // Builds the sources for the host with src/core/host_render.cc as the runtime, runs the
    // program once and writes the HTML of its first render to out_html. Needs a host clang++.
    bool render_first_frame(const SchemaDefs &defs, const std::vector<std::string> &input_files,
                            const std::string &cache_dir, const std::string &out_html);
}
