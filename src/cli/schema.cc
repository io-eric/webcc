#include "schema.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <cstring>

namespace webcc
{
    // Binary cache magic and version for validation
    static constexpr uint32_t SCHEMA_MAGIC = 0x57434353; // "WCCS" (WebCC Schema)
    static constexpr uint32_t SCHEMA_VERSION = 1;

    // Helper functions for binary serialization
    static void write_string(std::ostream &out, const std::string &s)
    {
        uint32_t len = static_cast<uint32_t>(s.size());
        out.write(reinterpret_cast<const char *>(&len), sizeof(len));
        out.write(s.data(), len);
    }

    static std::string read_string(std::istream &in)
    {
        uint32_t len;
        in.read(reinterpret_cast<char *>(&len), sizeof(len));
        std::string s(len, '\0');
        in.read(&s[0], len);
        return s;
    }

    static void write_param(std::ostream &out, const SchemaParam &p)
    {
        write_string(out, p.type);
        write_string(out, p.name);
        write_string(out, p.handle_type);
    }

    static SchemaParam read_param(std::istream &in)
    {
        SchemaParam p;
        p.type = read_string(in);
        p.name = read_string(in);
        p.handle_type = read_string(in);
        return p;
    }

    bool save_defs_binary(const SchemaDefs &defs, const std::string &path)
    {
        std::ofstream out(path, std::ios::binary);
        if (!out)
        {
            std::cerr << "[WebCC] Error: Could not write binary cache: " << path << std::endl;
            return false;
        }

        // Header
        out.write(reinterpret_cast<const char *>(&SCHEMA_MAGIC), sizeof(SCHEMA_MAGIC));
        out.write(reinterpret_cast<const char *>(&SCHEMA_VERSION), sizeof(SCHEMA_VERSION));

        // Inheritance map
        uint32_t inherit_count = static_cast<uint32_t>(defs.handle_inheritance.size());
        out.write(reinterpret_cast<const char *>(&inherit_count), sizeof(inherit_count));
        for (const auto &kv : defs.handle_inheritance)
        {
            write_string(out, kv.first);
            write_string(out, kv.second);
        }

        // Commands
        uint32_t cmd_count = static_cast<uint32_t>(defs.commands.size());
        out.write(reinterpret_cast<const char *>(&cmd_count), sizeof(cmd_count));
        for (const auto &c : defs.commands)
        {
            write_string(out, c.ns);
            write_string(out, c.name);
            out.write(reinterpret_cast<const char *>(&c.opcode), sizeof(c.opcode));
            write_string(out, c.func_name);
            write_string(out, c.action);
            write_string(out, c.return_type);
            write_string(out, c.return_handle_type);

            uint32_t param_count = static_cast<uint32_t>(c.params.size());
            out.write(reinterpret_cast<const char *>(&param_count), sizeof(param_count));
            for (const auto &p : c.params)
            {
                write_param(out, p);
            }
        }

        // Events
        uint32_t event_count = static_cast<uint32_t>(defs.events.size());
        out.write(reinterpret_cast<const char *>(&event_count), sizeof(event_count));
        for (const auto &e : defs.events)
        {
            write_string(out, e.ns);
            write_string(out, e.name);
            out.write(reinterpret_cast<const char *>(&e.opcode), sizeof(e.opcode));

            uint32_t param_count = static_cast<uint32_t>(e.params.size());
            out.write(reinterpret_cast<const char *>(&param_count), sizeof(param_count));
            for (const auto &p : e.params)
            {
                write_param(out, p);
            }
        }

        std::cout << "[WebCC] Saved binary cache: " << path << std::endl;
        return true;
    }

    bool load_defs_binary(SchemaDefs &defs, const std::string &path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            return false;
        }

        // Validate header
        uint32_t magic, version;
        in.read(reinterpret_cast<char *>(&magic), sizeof(magic));
        in.read(reinterpret_cast<char *>(&version), sizeof(version));

        if (magic != SCHEMA_MAGIC)
        {
            std::cerr << "[WebCC] Warning: Invalid cache magic, will regenerate" << std::endl;
            return false;
        }
        if (version != SCHEMA_VERSION)
        {
            std::cerr << "[WebCC] Warning: Cache version mismatch, will regenerate" << std::endl;
            return false;
        }

        // Inheritance map
        uint32_t inherit_count;
        in.read(reinterpret_cast<char *>(&inherit_count), sizeof(inherit_count));
        for (uint32_t i = 0; i < inherit_count; ++i)
        {
            std::string key = read_string(in);
            std::string value = read_string(in);
            defs.handle_inheritance[key] = value;
        }

        // Commands
        uint32_t cmd_count;
        in.read(reinterpret_cast<char *>(&cmd_count), sizeof(cmd_count));
        defs.commands.reserve(cmd_count);
        for (uint32_t i = 0; i < cmd_count; ++i)
        {
            SchemaCommand c;
            c.ns = read_string(in);
            c.name = read_string(in);
            in.read(reinterpret_cast<char *>(&c.opcode), sizeof(c.opcode));
            c.func_name = read_string(in);
            c.action = read_string(in);
            c.return_type = read_string(in);
            c.return_handle_type = read_string(in);

            uint32_t param_count;
            in.read(reinterpret_cast<char *>(&param_count), sizeof(param_count));
            c.params.reserve(param_count);
            for (uint32_t j = 0; j < param_count; ++j)
            {
                c.params.push_back(read_param(in));
            }
            defs.commands.push_back(std::move(c));
        }

        // Events
        uint32_t event_count;
        in.read(reinterpret_cast<char *>(&event_count), sizeof(event_count));
        defs.events.reserve(event_count);
        for (uint32_t i = 0; i < event_count; ++i)
        {
            SchemaEvent e;
            e.ns = read_string(in);
            e.name = read_string(in);
            in.read(reinterpret_cast<char *>(&e.opcode), sizeof(e.opcode));

            uint32_t param_count;
            in.read(reinterpret_cast<char *>(&param_count), sizeof(param_count));
            e.params.reserve(param_count);
            for (uint32_t j = 0; j < param_count; ++j)
            {
                e.params.push_back(read_param(in));
            }
            defs.events.push_back(std::move(e));
        }

        if (!in)
        {
            std::cerr << "[WebCC] Warning: Corrupt cache file" << std::endl;
            defs = SchemaDefs();
            return false;
        }

        std::cout << "[WebCC] Loaded from binary cache: " << defs.commands.size() << " commands, " << defs.events.size() << " events" << std::endl;
        return true;
    }

    SchemaDefs load_defs_cached(const std::string &cache_path, const std::string &def_path)
    {
        SchemaDefs defs;

        // Try binary cache first
        if (load_defs_binary(defs, cache_path))
        {
            return defs;
        }

        // Fall back to text parsing if def_path is provided
        if (!def_path.empty())
        {
            defs = load_defs(def_path);
            // Save cache for next time
            save_defs_binary(defs, cache_path);
            return defs;
        }

        std::cerr << "[WebCC] Error: No binary cache found and no schema.def path provided" << std::endl;
        exit(1);
    }

    SchemaDefs load_defs(const std::string &path)
    {
        std::cout << "[WebCC] Loading definitions from " << path << std::endl;
        SchemaDefs out;
        std::string contents = read_file(path);
        if (contents.empty())
        {
            std::cerr << "[WebCC] Error: Definition file is empty or missing: " << path << std::endl;
            exit(1);
        }
        std::istringstream ss(contents);
        std::string line;
        uint8_t current_cmd_opcode = 1;
        uint8_t current_event_opcode = 1;
        int line_num = 0;

        while (std::getline(ss, line))
        {
            line_num++;
            // trim
            size_t p = line.find_first_not_of(" \t\r\n");
            if (p == std::string::npos)
                continue;
            if (line[p] == '#')
                continue;
            // split by '|'
            auto parts = std::vector<std::string>();
            size_t start = 0;
            // We expect at least 4 separators
            while (true)
            {
                size_t pos = line.find('|', start);
                if (pos == std::string::npos)
                {
                    parts.push_back(line.substr(start));
                    break;
                }
                parts.push_back(line.substr(start, pos - start));
                start = pos + 1;
            }

            if (parts.size() < 4)
            {
                std::cerr << "[WebCC] Warning: Skipping malformed line " << line_num << ": " << line << std::endl;
                continue;
            }

            std::string ns = parts[0];
            std::string kind = "command";
            int name_idx = 1;

            if (ns == "meta") {
                if (parts.size() >= 4 && parts[1] == "inherit") {
                    // meta|inherit|Derived|Base
                    out.handle_inheritance[parts[2]] = parts[3];
                }
                continue;
            }

            // Check if second column is explicit kind
            if (parts[1] == "event" || parts[1] == "command")
            {
                kind = parts[1];
                name_idx = 2;
            }

            if (kind == "event")
            {
                // NAMESPACE|event|NAME|ARGS
                if (parts.size() <= name_idx + 1)
                    continue;
                SchemaEvent e;
                e.ns = ns;
                e.name = parts[name_idx];
                e.opcode = current_event_opcode++;

                std::istringstream tss(parts[name_idx + 1]);
                std::string tkn;
                while (tss >> tkn)
                {
                    SchemaParam p;
                    size_t colon = tkn.find(':');
                    if (colon == std::string::npos)
                    {
                        p.type = tkn;
                        p.name = "";
                    }
                    else
                    {
                        p.type = tkn.substr(0, colon);
                        p.name = tkn.substr(colon + 1);
                    }

                    // Check for handle(TypeName) syntax
                    if (p.type.substr(0, 7) == "handle(")
                    {
                        size_t close_paren = p.type.find(')');
                        if (close_paren != std::string::npos)
                        {
                            p.handle_type = p.type.substr(7, close_paren - 7);
                            p.type = "handle";
                        }
                    }

                    e.params.push_back(p);
                }
                out.events.push_back(e);
            }
            else
            {
                // NAMESPACE|[command]|NAME|FUNC_NAME|TYPES|ACTION
                if (parts.size() <= name_idx + 3)
                    continue;
                SchemaCommand c;
                c.ns = ns;
                c.name = parts[name_idx];
                c.opcode = current_cmd_opcode++;
                c.func_name = parts[name_idx + 1];
                // types
                std::istringstream tss(parts[name_idx + 2]);
                std::string tkn;
                while (tss >> tkn)
                {
                    SchemaParam p;
                    size_t colon = tkn.find(':');
                    if (colon == std::string::npos)
                    {
                        p.type = tkn;
                        p.name = "";
                    }
                    else
                    {
                        p.type = tkn.substr(0, colon);
                        p.name = tkn.substr(colon + 1);
                    }

                    // Check for handle(TypeName) syntax
                    if (p.type.substr(0, 7) == "handle(")
                    {
                        size_t close_paren = p.type.find(')');
                        if (close_paren != std::string::npos)
                        {
                            p.handle_type = p.type.substr(7, close_paren - 7);
                            p.type = "handle";
                        }
                    }
                    // Also check name for handle(TypeName) syntax (for RET:handle(TypeName))
                    if (p.name.substr(0, 7) == "handle(")
                    {
                        size_t close_paren = p.name.find(')');
                        if (close_paren != std::string::npos)
                        {
                            p.handle_type = p.name.substr(7, close_paren - 7);
                            p.name = "handle";
                        }
                    }

                    if (p.type == "RET")
                    {
                        c.return_type = p.name;
                        if (!p.handle_type.empty())
                        {
                            c.return_handle_type = p.handle_type;
                        }
                    }
                    else
                    {
                        c.params.push_back(p);
                    }
                }
                // Reconstruct action by finding the start position in the line
                size_t action_pos = 0;
                int pipes_needed = name_idx + 3;
                for (int i = 0; i < pipes_needed; ++i)
                {
                    action_pos = line.find('|', action_pos) + 1;
                }
                c.action = line.substr(action_pos);
                out.commands.push_back(c);
            }
        }
        std::cout << "[WebCC] Loaded " << out.commands.size() << " commands and " << out.events.size() << " events." << std::endl;
        return out;
    }

    SchemaDefs load_defs_from_schema()
    {
        // Legacy function - now just returns empty, use load_defs_cached instead
        std::cerr << "[WebCC] Warning: load_defs_from_schema() is deprecated, use load_defs_cached()" << std::endl;
        return SchemaDefs();
    }

} // namespace webcc
