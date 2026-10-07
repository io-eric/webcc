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
        std::string default_value; // C++ default arg, commands only
        std::string enum_type;     // group name, type is its wire type
    };

    // NAMESPACE|enum|Name:wire|A B C       choices, numbered 0, 1, 2...
    // NAMESPACE|flags|Name:wire|A=1 B=2    bit flags, combined with |
    struct SchemaGroup
    {
        std::string ns;
        std::string name;
        std::string wire; // uint8, uint32 or int32
        bool flags = false;
        std::vector<std::pair<std::string, std::string>> values; // name, value
    };

    // NAMESPACE|const|NAME|VALUE
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
        std::string return_enum_type;    // set when RET is a group
    };

    // Represents an event definition from `schema.def`.
    struct SchemaEvent
    {
        std::string ns;
        std::string name;
        // no more events for the handle in the first field
        bool last = false;
        uint8_t opcode; // one byte in the event header
        std::vector<SchemaParam> params;
    };

    // Holds all command and event definitions.
    // NAMESPACE|helper|NAME|path.js, path relative to schema.def
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
        std::vector<SchemaGroup> groups;
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
