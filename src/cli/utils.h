#pragma once
#include <iostream>
#include <string>
#include <string>
#include <sstream>

namespace webcc
{
    // --quiet: no progress lines on stdout, errors and warnings still go to stderr
    inline bool quiet = false;
    // --dev: a build that is quick to make rather than small or fast: -O0, no link-time optimization
    inline bool dev = false;
    // --progress: one ">> step" line per step on stdout, for a tool that shows the current step
    inline bool progress = false;
    inline void report(const std::string& step)
    {
        if (progress) std::cout << ">> " << step << std::endl;
    }


  // Reads the entire contents of a file into a string.
  std::string read_file(const std::string &path);

  // Gets the full path to the currently running executable.
  std::string get_executable_path();

  // Gets the directory where the currently running executable is located.
  std::string get_executable_dir();

  // Writes a string to a file, creating parent directories if they don't exist.
  bool write_file(const std::string &path, const std::string &contents);

  // A helper class to write code with automatic indentation.
  class CodeWriter
  {
  public:
    CodeWriter() = default;

    // Writes a line with automatic indentation based on braces.
    void write(const std::string &line);

    // Writes raw text without modifying indentation (useful for templates).
    void raw(const std::string &text);

    // Manually adjust indentation level.
    void indent();
    void dedent();
    void set_indent(int level);

    // Returns the generated code.
    std::string str() const;

  private:
    std::stringstream buffer_;
    int indent_level_ = 0;
  };

} // namespace webcc
