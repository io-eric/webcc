#include "schema.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <map>
#include <set>
#include <cctype>

namespace webcc
{
    // Binary cache magic and version for validation
    static constexpr uint32_t SCHEMA_MAGIC = 0x57434353; // "WCCS" (WebCC Schema)
    static constexpr uint32_t SCHEMA_VERSION = 6;

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
        write_string(out, p.default_value);
        write_string(out, p.enum_type);
    }

    static SchemaParam read_param(std::istream &in)
    {
        SchemaParam p;
        p.type = read_string(in);
        p.name = read_string(in);
        p.handle_type = read_string(in);
        p.default_value = read_string(in);
        p.enum_type = read_string(in);
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
            write_string(out, c.return_enum_type);

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
            out.put(e.last ? 1 : 0);

            uint32_t param_count = static_cast<uint32_t>(e.params.size());
            out.write(reinterpret_cast<const char *>(&param_count), sizeof(param_count));
            for (const auto &p : e.params)
            {
                write_param(out, p);
            }
        }

        // Constants
        uint32_t const_count = static_cast<uint32_t>(defs.consts.size());
        out.write(reinterpret_cast<const char *>(&const_count), sizeof(const_count));
        for (const auto &k : defs.consts)
        {
            write_string(out, k.ns);
            write_string(out, k.name);
            write_string(out, k.value);
        }

        // Enum and flags groups
        uint32_t group_count = static_cast<uint32_t>(defs.groups.size());
        out.write(reinterpret_cast<const char *>(&group_count), sizeof(group_count));
        for (const auto &g : defs.groups)
        {
            write_string(out, g.ns);
            write_string(out, g.name);
            write_string(out, g.wire);
            out.put(g.flags ? 1 : 0);
            uint32_t n = static_cast<uint32_t>(g.values.size());
            out.write(reinterpret_cast<const char *>(&n), sizeof(n));
            for (const auto &[name, value] : g.values)
            {
                write_string(out, name);
                write_string(out, value);
            }
        }

        // JS helpers
        uint32_t helper_count = static_cast<uint32_t>(defs.helpers.size());
        out.write(reinterpret_cast<const char *>(&helper_count), sizeof(helper_count));
        for (const auto &h : defs.helpers)
        {
            write_string(out, h.ns);
            write_string(out, h.name);
            write_string(out, h.code);
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
            c.return_enum_type = read_string(in);

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
            e.last = in.get() != 0;

            uint32_t param_count;
            in.read(reinterpret_cast<char *>(&param_count), sizeof(param_count));
            e.params.reserve(param_count);
            for (uint32_t j = 0; j < param_count; ++j)
            {
                e.params.push_back(read_param(in));
            }
            defs.events.push_back(std::move(e));
        }

        // Constants
        uint32_t const_count = 0;
        in.read(reinterpret_cast<char *>(&const_count), sizeof(const_count));
        for (uint32_t i = 0; in && i < const_count; ++i)
        {
            SchemaConst k;
            k.ns = read_string(in);
            k.name = read_string(in);
            k.value = read_string(in);
            defs.consts.push_back(std::move(k));
        }

        // Enum and flags groups
        uint32_t group_count = 0;
        in.read(reinterpret_cast<char *>(&group_count), sizeof(group_count));
        for (uint32_t i = 0; in && i < group_count; ++i)
        {
            SchemaGroup g;
            g.ns = read_string(in);
            g.name = read_string(in);
            g.wire = read_string(in);
            g.flags = in.get() != 0;
            uint32_t n = 0;
            in.read(reinterpret_cast<char *>(&n), sizeof(n));
            for (uint32_t j = 0; in && j < n; ++j)
            {
                std::string name = read_string(in);
                std::string value = read_string(in);
                g.values.push_back({name, value});
            }
            defs.groups.push_back(std::move(g));
        }

        // JS helpers
        uint32_t helper_count = 0;
        in.read(reinterpret_cast<char *>(&helper_count), sizeof(helper_count));
        for (uint32_t i = 0; in && i < helper_count; ++i)
        {
            SchemaHelper h;
            h.ns = read_string(in);
            h.name = read_string(in);
            h.code = read_string(in);
            defs.helpers.push_back(std::move(h));
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
        uint32_t current_cmd_opcode = 1;
        uint32_t current_event_opcode = 1;
        int line_num = 0;
        
        // Track seen names per namespace for duplicate detection
        // Maps namespace -> set of command/event names
        std::map<std::string, std::set<std::string>> seen_commands;
        std::map<std::string, std::set<std::string>> seen_events;
        // Track func_names per namespace (they become C++ functions in that namespace)
        std::map<std::string, std::set<std::string>> seen_func_names;

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

            // NAMESPACE|helper|NAME|path.js
            if (parts[1] == "helper")
            {
                std::string dir = path.substr(0, path.find_last_of('/') + 1);
                std::string code = read_file(dir + parts[3]);
                if (code.empty())
                {
                    std::cerr << "[WebCC] Error: Helper file '" << parts[3] << "' missing or empty at line " << line_num << std::endl;
                    exit(1);
                }
                for (const auto &h : out.helpers)
                {
                    if (h.name == parts[2])
                    {
                        std::cerr << "[WebCC] Error: Duplicate helper '" << parts[2] << "' at line " << line_num << std::endl;
                        exit(1);
                    }
                }
                out.helpers.push_back({ns, parts[2], code});
                continue;
            }

            // NAMESPACE|enum|Name:wire|A B C  /  NAMESPACE|flags|Name:wire|A=1 B=2
            if (parts[1] == "enum" || parts[1] == "flags")
            {
                SchemaGroup g;
                g.ns = ns;
                g.flags = parts[1] == "flags";
                size_t colon = parts[2].find(':');
                g.name = parts[2].substr(0, colon);
                g.wire = colon == std::string::npos ? "uint8" : parts[2].substr(colon + 1);
                if (g.wire != "uint8" && g.wire != "uint32" && g.wire != "int32")
                {
                    std::cerr << "[WebCC] Error: " << parts[1] << " '" << g.name << "' needs a wire type uint8, uint32 or int32 at line " << line_num << std::endl;
                    exit(1);
                }
                if (g.name.empty() || !std::isupper((unsigned char)g.name[0]))
                {
                    std::cerr << "[WebCC] Error: " << parts[1] << " name must be UpperCamelCase at line " << line_num << std::endl;
                    exit(1);
                }
                for (const auto &other : out.groups)
                {
                    if (other.name == g.name)
                    {
                        std::cerr << "[WebCC] Error: Duplicate enum/flags name '" << g.name << "' at line " << line_num << " (names are unique across namespaces)" << std::endl;
                        exit(1);
                    }
                }
                std::istringstream vs(parts[3]);
                std::string v;
                int next = 0;
                while (vs >> v)
                {
                    size_t eq = v.find('=');
                    std::string vname = v.substr(0, eq);
                    std::string value;
                    if (g.flags)
                    {
                        if (eq == std::string::npos)
                        {
                            std::cerr << "[WebCC] Error: flags value '" << vname << "' needs =value at line " << line_num << std::endl;
                            exit(1);
                        }
                        value = v.substr(eq + 1);
                    }
                    else
                    {
                        // numbered in order
                        if (eq != std::string::npos)
                        {
                            std::cerr << "[WebCC] Error: enum values are numbered in order, no =value ('" << v << "') at line " << line_num << std::endl;
                            exit(1);
                        }
                        value = std::to_string(next++);
                    }
                    g.values.push_back({vname, value});
                }
                if (g.values.empty())
                {
                    std::cerr << "[WebCC] Error: " << parts[1] << " '" << g.name << "' has no values at line " << line_num << std::endl;
                    exit(1);
                }
                out.groups.push_back(std::move(g));
                continue;
            }

            // NAMESPACE|const|NAME|VALUE
            if (parts[1] == "const")
            {
                const std::string &name = parts[2];
                const std::string &value = parts[3];
                bool upper = !name.empty();
                for (char ch : name)
                    if (!(std::isupper((unsigned char)ch) || std::isdigit((unsigned char)ch) || ch == '_'))
                        upper = false;
                if (!upper || value.empty())
                {
                    std::cerr << "[WebCC] Error: Constant must be UPPER_CASE with a value at line " << line_num << std::endl;
                    exit(1);
                }
                for (const auto &k : out.consts)
                {
                    if (k.ns == ns && k.name == name)
                    {
                        std::cerr << "[WebCC] Error: Duplicate constant '" << name << "' in namespace '" << ns << "' at line " << line_num << std::endl;
                        exit(1);
                    }
                }
                out.consts.push_back({ns, name, value});
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
                
                std::string event_name = parts[name_idx];
                
                // Check for duplicate event name in this namespace
                if (seen_events[ns].count(event_name))
                {
                    std::cerr << "[WebCC] Error: Duplicate event name '" << event_name 
                              << "' in namespace '" << ns << "' at line " << line_num << std::endl;
                    exit(1);
                }
                seen_events[ns].insert(event_name);
                
                if (current_event_opcode > 0xFF)
                {
                    std::cerr << "[WebCC] Error: Too many events (max 255) at line " << line_num << std::endl;
                    exit(1);
                }

                SchemaEvent e;
                e.ns = ns;
                e.name = event_name;
                e.opcode = static_cast<uint8_t>(current_event_opcode++);
                if (parts.size() > (size_t)name_idx + 2)
                {
                    std::string flag = parts[name_idx + 2];
                    flag.erase(0, flag.find_first_not_of(" \t"));
                    flag.erase(flag.find_last_not_of(" \t\r") + 1);
                    if (flag == "last")
                        e.last = true;
                    else if (!flag.empty())
                    {
                        std::cerr << "[WebCC] Error: Unknown event flag '" << flag << "' at line " << line_num << " (only 'last')" << std::endl;
                        exit(1);
                    }
                }

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
                
                std::string cmd_name = parts[name_idx];
                
                // Check for duplicate command name in this namespace
                if (seen_commands[ns].count(cmd_name))
                {
                    std::cerr << "[WebCC] Error: Duplicate command name '" << cmd_name 
                              << "' in namespace '" << ns << "' at line " << line_num << std::endl;
                    exit(1);
                }
                seen_commands[ns].insert(cmd_name);
                
                std::string func_name = parts[name_idx + 1];
                
                // Check for duplicate func_name within the same namespace
                if (seen_func_names[ns].count(func_name))
                {
                    std::cerr << "[WebCC] Error: Duplicate func_name '" << func_name 
                              << "' in namespace '" << ns << "' at line " << line_num << std::endl;
                    exit(1);
                }
                seen_func_names[ns].insert(func_name);
                
                SchemaCommand c;
                c.ns = ns;
                c.name = cmd_name;
                if (current_cmd_opcode > 0xFFFF)
                {
                    std::cerr << "[WebCC] Error: Too many commands (max 65535) at line " << line_num << std::endl;
                    exit(1);
                }
                c.opcode = static_cast<uint16_t>(current_cmd_opcode++);
                c.func_name = func_name;
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

                    // Default argument: type:name=value
                    size_t eq = p.name.find('=');
                    if (eq != std::string::npos)
                    {
                        p.default_value = p.name.substr(eq + 1);
                        p.name = p.name.substr(0, eq);
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

                // defaults must be trailing
                bool seen_default = false;
                for (const auto &p : c.params)
                {
                    if (!p.default_value.empty())
                        seen_default = true;
                    else if (seen_default)
                    {
                        std::cerr << "[WebCC] Error: Parameter '" << p.name << "' of '" << c.func_name
                                  << "' needs a default because an earlier one has one, at line " << line_num << std::endl;
                        exit(1);
                    }
                }
                out.commands.push_back(c);
            }
        }
        std::cout << "[WebCC] Loaded " << out.commands.size() << " commands and " << out.events.size() << " events." << std::endl;
        // group-typed params, fields and returns: wire as the group's type
        {
            static const std::set<std::string> base = {"string", "bytes", "handle", "int32", "uint32", "float32", "float64", "uint8", "func_ptr"};
            auto resolve = [&](std::string &type, std::string &enum_type, const std::string &where) {
                if (type.empty() || base.count(type))
                    return;
                for (const auto &g : out.groups)
                {
                    if (g.name == type)
                    {
                        enum_type = g.ns + "::" + g.name;
                        type = g.wire;
                        return;
                    }
                }
                std::cerr << "[WebCC] Error: Unknown type '" << type << "' in " << where << std::endl;
                exit(1);
            };
            for (auto &cmd : out.commands)
            {
                for (auto &p : cmd.params)
                    resolve(p.type, p.enum_type, cmd.ns + "::" + cmd.func_name);
                resolve(cmd.return_type, cmd.return_enum_type, cmd.ns + "::" + cmd.func_name + " return");
            }
            for (auto &e : out.events)
                for (auto &p : e.params)
                    resolve(p.type, p.enum_type, e.ns + " event " + e.name);
        }

        return out;
    }

    SchemaDefs load_defs_from_schema()
    {
        // Legacy function - now just returns empty, use load_defs_cached instead
        std::cerr << "[WebCC] Warning: load_defs_from_schema() is deprecated, use load_defs_cached()" << std::endl;
        return SchemaDefs();
    }

} // namespace webcc
