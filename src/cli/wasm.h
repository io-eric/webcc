#pragma once
#include <string>
#include <set>

namespace webcc
{

    // Parses the Import section of a WebAssembly module and collects the field
    // names imported from the given module namespace (default "env").
    //
    // This is the ground-truth source for feature detection: every webcc API the
    // user's code actually references appears here as a `webcc_*` import, resolved
    // by the C++ compiler/linker rather than guessed from raw source text.
    //
    // Returns false if the file is missing or not a well-formed wasm module; on
    // false, `out` should be treated as invalid.
    bool read_wasm_imports(const std::string &path, std::set<std::string> &out,
                           const std::string &module_filter = "env");

} // namespace webcc
