#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

std::string read_file(const std::string &path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in)
        return std::string();
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
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
    size_t pos = path.find_last_of("/");
    if (pos != std::string::npos)
    {
        return path.substr(0, pos);
    }
    return ".";
}

bool write_file(const std::string &path, const std::string &contents)
{
    // Ensure parent directories exist roughly (best-effort)
    size_t pos = path.find_last_of("/");
    if (pos != std::string::npos)
    {
        std::string dir = path.substr(0, pos);
        // try creating directory; ignore errors
        mkdir(dir.c_str(), 0755);
    }
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out)
        return false;
    out << contents;
    return out.good();
}
