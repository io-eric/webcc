# Contributing to WebCC

First off, thanks for taking the time to contribute! 🎉

The following is a set of guidelines for contributing to WebCC. These are mostly guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request.

## Code of Conduct

This project and everyone participating in it is governed by the [WebCC Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs

This section guides you through submitting a bug report for WebCC. Following these guidelines helps maintainers and the community understand your report, reproduce the behavior, and find related reports.

- **Use a clear and descriptive title** for the issue to identify the problem.
- **Describe the exact steps to reproduce the problem** in as many details as possible.
- **Provide specific examples** to demonstrate the steps.
- **Describe the behavior you observed** after following the steps and point out what exactly is the problem with that behavior.
- **Explain which behavior you expected to see instead** and why.

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for WebCC, including completely new features and minor improvements to existing functionality.

- **Use a clear and descriptive title** for the issue to identify the suggestion.
- **Provide a step-by-step description of the suggested enhancement** in as many details as possible.
- **Explain why this enhancement would be useful** to most WebCC users.

### Pull Requests

- Fill in the required template
- Do not include issue numbers in the PR title
- Include screenshots and animated GIFs in your pull request whenever possible.
- Follow the C++ style of the existing codebase.
- Make sure your code builds and works by running `./test_examples.sh`.

## Style Guide

- **Naming**: Use `snake_case` for variables and functions. Use `PascalCase` for classes and structs, except for core library types that mimic the standard library (e.g., `string`, `unique_ptr`), which use `snake_case`.
- **C++ Standard**: We use C++20.
- **Consistency**: Follow the style of the surrounding code.
- **Generated Files**: Generated files (like headers) must start with a comment indicating they are generated (e.g., `// GENERATED FILE - DO NOT EDIT`).

## Development Setup

1.  Clone the repo.
2.  Run `./build.sh` to bootstrap the compiler.
    - This compiles the `webcc` tool itself.
    - It generates headers in `include/webcc/` from `schema.def`.
3.  Make your changes.
    - If you modify `schema.def`, run `./build.sh` again to regenerate headers and the tool.
    - If you modify C++ source files in `src/`, run `./build.sh` to rebuild the tool.
4.  Test with `./test_examples.sh`.
    - This builds all examples in `examples/`.
    - It starts a local server at `http://localhost:8000`.

