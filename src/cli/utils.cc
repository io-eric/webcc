#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace webcc
{

    std::string read_file(const std::string &path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec))
            return {};

        auto file_size = std::filesystem::file_size(path, ec);
        if (ec)
            return {};

        std::string content(file_size, '\0');
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.read(content.data(), file_size))
            return {};

        return content;
    }

    std::string get_executable_path()
    {
#ifdef __APPLE__
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0)
        {
            return std::string(path);
        }
        return "";
#else
        // Linux and WSL (Windows Subsystem for Linux)
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count != -1)
        {
            return std::string(result, count);
        }
        return "";
#endif
    }

    std::string get_executable_dir()
    {
        std::string path = get_executable_path();
        if (path.empty())
            return ".";
        return std::filesystem::path(path).parent_path().string();
    }

    bool write_file(const std::string &path, const std::string &contents)
    {
        std::filesystem::path p(path);
        if (p.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
            if (ec)
                return false;
        }

        std::ofstream out(path, std::ios::out | std::ios::binary);
        if (!out)
            return false;
        out.write(contents.data(), contents.size());
        return out.good();
    }

    void CodeWriter::write(const std::string &text)
    {
        if (text.empty())
        {
            buffer_ << "\n";
            return;
        }
        std::stringstream ss(text);
        std::string line;
        while (std::getline(ss, line))
        {
            size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos)
            {
                buffer_ << "\n";
                continue;
            }

            std::string trimmed = line.substr(first);
            size_t last = trimmed.find_last_not_of(" \t\r");
            if (last != std::string::npos)
            {
                trimmed = trimmed.substr(0, last + 1);
            }

            if (trimmed.front() == '}')
            {
                if (indent_level_ > 0)
                    indent_level_--;
            }

            for (int i = 0; i < indent_level_; ++i)
            {
                buffer_ << "    ";
            }

            buffer_ << trimmed << "\n";

            if (trimmed.back() == '{')
            {
                indent_level_++;
            }
        }
    }

    void CodeWriter::raw(const std::string &text)
    {
        buffer_ << text;
    }

    void CodeWriter::indent()
    {
        indent_level_++;
    }

    void CodeWriter::dedent()
    {
        if (indent_level_ > 0)
            indent_level_--;
    }

    void CodeWriter::set_indent(int level)
    {
        indent_level_ = level;
    }

    std::string CodeWriter::str() const
    {
        return buffer_.str();
    }

} // namespace webcc
