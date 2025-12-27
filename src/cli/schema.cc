#include "schema.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

#if __has_include("webcc_schema.h")
#include "webcc_schema.h"
#define WEBCC_HAS_SCHEMA 1
#else
#define WEBCC_HAS_SCHEMA 0
#endif

namespace webcc
{

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

                    if (p.type == "RET")
                    {
                        c.return_type = p.name;
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
#if WEBCC_HAS_SCHEMA
        std::cout << "[WebCC] Loading definitions from compiled-in schema..." << std::endl;
        SchemaDefs out;

        for (const auto *c = webcc::SCHEMA_COMMANDS; !c->ns.empty(); ++c)
        {
            out.commands.push_back(*c);
        }

        for (const auto *e = webcc::SCHEMA_EVENTS; !e->ns.empty(); ++e)
        {
            out.events.push_back(*e);
        }

        std::cout << "[WebCC] Loaded " << out.commands.size() << " commands and " << out.events.size() << " events." << std::endl;
        return out;
#else
        return SchemaDefs();
#endif
    }

} // namespace webcc
