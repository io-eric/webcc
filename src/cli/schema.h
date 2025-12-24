#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Represents a parameter for a command or event.
struct Param
{
    std::string type;
    std::string name; // optional; if empty we'll generate argN
};

// Represents a command definition from `schema.def`.
struct CommandDef
{
    std::string ns;   // Namespace
    std::string name; // NAME token
    uint8_t opcode;
    std::string func_name;     // C++ function name to search for
    std::vector<Param> params; // list of parameters
    std::string action;        // JS action body (using arg0.. or custom names)
    std::string return_type;   // Optional return type
};

// Represents an event definition from `schema.def`.
struct EventDef
{
    std::string ns;
    std::string name;
    uint8_t opcode;
    std::vector<Param> params;
};

// Holds all command and event definitions.
struct Defs
{
    std::vector<CommandDef> commands;
    std::vector<EventDef> events;
};

// Loads and parses the command and event definitions from a file (e.g., schema.def).
Defs load_defs(const std::string &path);

// Loads definitions from the compiled-in schema header.
Defs load_defs_from_schema();
