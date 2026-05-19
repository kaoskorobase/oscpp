# ADR-0001: Python for README code-extraction script

**Status:** Accepted  
**Date:** 2026-05-19

## Context

The build system needs a script to extract C++ code blocks from `README.md` and concatenate them into a compilable `.cpp` file for the README smoke test. The existing script is Ruby (`tools/mdcode.rb`). Three alternatives were evaluated:

- **Ruby** (existing): not guaranteed on CI runners; adds an undeclared dependency.
- **Pure CMake** (`cmake -P`): zero external dependencies, but `file(STRINGS)` strips backslashes (corrupts C++ string literals); `file(READ)` + regex is safe but harder to maintain.
- **Python 3**: available on all modern CI runners and macOS by default; stdlib only; idiomatic and easy to modify.

## Decision

Replace `tools/mdcode.rb` with `tools/mdcode.py` using Python 3 stdlib only.

## Consequences

- CI matrix jobs require Python 3 (available by default on `ubuntu-latest` and `macos-latest` GitHub Actions runners — no install step needed).
- CMakeLists.txt uses `find_package(Python3 REQUIRED COMPONENTS Interpreter)` to locate the interpreter.
- The script remains easy to read and modify without CMake scripting knowledge.
