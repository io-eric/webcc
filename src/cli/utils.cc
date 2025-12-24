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

std::string read_file(const std::string &path)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec))
        return {};

    auto file_size = std::filesystem::file_size(path, ec);
    if (ec) return {};

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
    if (path.empty()) return ".";
    return std::filesystem::path(path).parent_path().string();
}

bool write_file(const std::string &path, const std::string &contents)
{
    std::filesystem::path p(path);
    if (p.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        if (ec) return false;
    }

    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out)
        return false;
    out.write(contents.data(), contents.size());
    return out.good();
}
