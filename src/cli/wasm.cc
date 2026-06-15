#include "wasm.h"
#include "utils.h"
#include <cstdint>

namespace webcc
{
    namespace
    {
        // Bounds-checked unsigned LEB128 decode. Advances `i`; false on
        // truncation or overflow.
        bool read_uleb(const std::string &b, size_t &i, uint64_t &out)
        {
            uint64_t result = 0;
            int shift = 0;
            while (true)
            {
                if (i >= b.size())
                    return false;
                if (shift >= 64)
                    return false; // malformed / overflow
                uint8_t byte = static_cast<uint8_t>(b[i++]);
                result |= static_cast<uint64_t>(byte & 0x7f) << shift;
                if ((byte & 0x80) == 0)
                    break;
                shift += 7;
            }
            out = result;
            return true;
        }

        // Read a length-prefixed byte vector (wasm `name`).
        bool read_name(const std::string &b, size_t &i, std::string &out)
        {
            uint64_t len;
            if (!read_uleb(b, i, len))
                return false;
            if (len > b.size() || i + len > b.size())
                return false;
            out.assign(b, i, static_cast<size_t>(len));
            i += static_cast<size_t>(len);
            return true;
        }
    } // namespace

    bool read_wasm_imports(const std::string &path, std::set<std::string> &out,
                           const std::string &module_filter)
    {
        std::string b = read_file(path);

        // Header: magic "\0asm" + 4-byte version.
        if (b.size() < 8)
            return false;
        if (static_cast<uint8_t>(b[0]) != 0x00 || b[1] != 'a' || b[2] != 's' || b[3] != 'm')
            return false;

        size_t i = 8;
        while (i < b.size())
        {
            uint8_t section_id = static_cast<uint8_t>(b[i++]);
            uint64_t section_len;
            if (!read_uleb(b, i, section_len))
                return false;
            if (section_len > b.size() || i + section_len > b.size())
                return false;
            size_t section_end = i + static_cast<size_t>(section_len);

            if (section_id == 2) // Import section
            {
                size_t j = i;
                uint64_t count;
                if (!read_uleb(b, j, count))
                    return false;

                for (uint64_t n = 0; n < count; ++n)
                {
                    std::string mod, field;
                    if (!read_name(b, j, mod))
                        return false;
                    if (!read_name(b, j, field))
                        return false;
                    if (j >= b.size())
                        return false;

                    uint8_t kind = static_cast<uint8_t>(b[j++]);
                    uint64_t tmp;
                    switch (kind)
                    {
                    case 0x00: // func: typeidx
                        if (!read_uleb(b, j, tmp))
                            return false;
                        break;
                    case 0x01: // table: reftype + limits
                    {
                        if (j >= b.size())
                            return false;
                        ++j; // reftype
                        if (j >= b.size())
                            return false;
                        uint8_t flags = static_cast<uint8_t>(b[j++]);
                        if (!read_uleb(b, j, tmp)) // min
                            return false;
                        if ((flags & 0x01) && !read_uleb(b, j, tmp)) // max
                            return false;
                        break;
                    }
                    case 0x02: // mem: limits
                    {
                        if (j >= b.size())
                            return false;
                        uint8_t flags = static_cast<uint8_t>(b[j++]);
                        if (!read_uleb(b, j, tmp)) // min
                            return false;
                        if ((flags & 0x01) && !read_uleb(b, j, tmp)) // max
                            return false;
                        break;
                    }
                    case 0x03: // global: valtype + mutability
                        if (j + 2 > b.size())
                            return false;
                        j += 2;
                        break;
                    default:
                        return false; // unknown import kind
                    }

                    if (mod == module_filter)
                        out.insert(field);
                }
            }

            i = section_end;
        }

        return true;
    }

} // namespace webcc
