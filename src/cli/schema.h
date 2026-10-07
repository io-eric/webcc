#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace webcc
{

    // Represents a parameter for a command or event.
    struct SchemaParam
    {
        std::string type;        // Base type: string, int32, handle, etc.
        std::string name;        // optional; if empty we'll generate argN
        std::string handle_type; // For handle types: DOMElement, CanvasContext2D, etc.
        std::string default_value; // C++ default argument, e.g. "0" or "\"\"" (commands only)
    };

    // Named constant from `NAMESPACE|const|NAME|VALUE`, emitted as constexpr in the namespace header.
    struct SchemaConst
    {
        std::string ns;
        std::string name;
        std::string value;
    };

    // Represents a command definition from `schema.def`.
    struct SchemaCommand
    {
        std::string ns;   // Namespace
        std::string name; // NAME token
        uint16_t opcode;
        std::string func_name;           // C++ function name to search for
        std::vector<SchemaParam> params; // list of parameters
        std::string action;              // JS action body (using arg0.. or custom names)
        std::string return_type;         // Optional return type: handle, int32, string, etc.
        std::string return_handle_type;  // For handle return types: DOMElement, CanvasContext2D, etc.
    };

    // Represents an event definition from `schema.def`.
    struct SchemaEvent
    {
        std::string ns;
        std::string name;
        // "last" in the 5th column: no more events follow for the handle in the first
        // field, so callbacks registered on it can be dropped
        bool last = false;
        uint8_t opcode; // one byte in the event header
        std::vector<SchemaParam> params;
    };

    // Holds all command and event definitions.
    // NAMESPACE|helper|NAME|path.js: a JS file (path relative to schema.def) that
    // defines NAME, for code too big for one action. Emitted once into app.js when a
    // used action mentions NAME.
    struct SchemaHelper
    {
        std::string ns;
        std::string name;
        std::string code;
    };

    struct SchemaDefs
    {
        std::vector<SchemaCommand> commands;
        std::vector<SchemaEvent> events;
        std::map<std::string, std::string> handle_inheritance;
        std::vector<SchemaConst> consts;
        std::vector<SchemaHelper> helpers;
    };

    // Loads and parses the command and event definitions from a file (e.g., schema.def).
    SchemaDefs load_defs(const std::string &path);

    // Loads definitions from the compiled-in schema header.
    SchemaDefs load_defs_from_schema();

    // Binary cache functions for fast loading without recompilation
    bool save_defs_binary(const SchemaDefs &defs, const std::string &path);
    bool load_defs_binary(SchemaDefs &defs, const std::string &path);
    
    // Try to load from binary cache, falling back to text parsing
    SchemaDefs load_defs_cached(const std::string &cache_path, const std::string &def_path = "");

} // namespace webcc
