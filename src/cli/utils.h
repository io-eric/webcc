#pragma once
#include <string>

// Reads the entire contents of a file into a string.
std::string read_file(const std::string &path);

// Gets the full path to the currently running executable.
std::string get_executable_path();

// Gets the directory where the currently running executable is located.
std::string get_executable_dir();

// Writes a string to a file, creating parent directories if they don't exist.
bool write_file(const std::string &path, const std::string &contents);
