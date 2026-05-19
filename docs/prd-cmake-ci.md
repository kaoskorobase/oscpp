# PRD: oscpp 1.0.0 — CMake Build System, Test Migration & CI

## Problem Statement

oscpp is a production-quality header-only C++11 OSC library used by audio software projects including methcla. Despite being mature and stable, it has no formal CMake build system, no tagged release, and no active CI. This means:

- Downstream consumers (e.g. methcla) cannot use standard CMake mechanisms (`FetchContent`, `find_package`) to depend on oscpp — they must vendor or hand-wire include paths.
- The existing property-based test suite depends on autocheck, an unmaintained library with no shrinking support, wired to a legacy Makefile build.
- There is no cross-platform verification — the Travis CI config is stale and non-functional.

## Solution

Add a proper CMake build system that exposes an `oscpp::oscpp` INTERFACE target consumable via `FetchContent` or `find_package`. Migrate the property-based tests from autocheck to RapidCheck + Catch2 v3, preserving both the identity property and the overflow property. Wire up a GitHub Actions CI matrix across macOS, Linux (GCC + Clang), and Windows (MSVC). Tag the result as `1.0.0`.

## User Stories

1. As a C++ developer, I want to consume oscpp via `FetchContent_MakeAvailable`, so that I don't need to vendor or hand-wire include paths.
2. As a C++ developer, I want to consume oscpp via `find_package(oscpp REQUIRED)`, so that I can use it after a `cmake --install` step.
3. As a C++ developer, I want `find_package(oscpp 1.0 REQUIRED)` to succeed and `find_package(oscpp 2.0 REQUIRED)` to fail with a version mismatch, so that I can guard against future breaking changes.
4. As a C++ developer consuming oscpp via FetchContent, I want oscpp's test dependencies (Catch2, RapidCheck) to not be fetched or compiled, so that my build is not slowed down by oscpp's internals.
5. As a C++ developer, I want `target_link_libraries(myapp PRIVATE oscpp::oscpp)` to automatically propagate the include path and C++11 requirement, so that I don't need to configure those manually.
6. As an oscpp developer, I want to run `cmake --preset debug && cmake --build build/debug` to get a working test build, so that the build setup is a single, documented command.
7. As an oscpp developer, I want to run `ctest --test-dir build/debug --output-on-failure` to run all tests, so that I get a clear pass/fail result across both test executables.
8. As an oscpp developer, I want the property-based tests to shrink counterexamples on failure, so that I get the minimal failing input rather than a large random one.
9. As an oscpp developer, I want `prop_identity` to verify that any generated OSC packet survives a round-trip through the client writer and server parser unchanged, so that encoding and decoding are jointly verified.
10. As an oscpp developer, I want `prop_overflow` to verify that writing a packet into an undersized buffer always raises `OSCPP::OverflowError`, so that error handling is property-tested.
11. As an oscpp developer, I want the README code examples to be compiled and run as a smoke test, so that the public API documentation is always valid.
12. As an oscpp developer, I want `-Wall -Wextra -Werror` applied to all test builds, so that warnings are caught before they accumulate.
13. As an oscpp developer, I want `cmake --preset release` to produce a build without test dependencies, so that I can verify a clean consumer build locally.
14. As an oscpp developer, I want the `test/autocheck` submodule and root `Makefile` removed, so that there is a single, canonical build path.
15. As a CI engineer, I want the GitHub Actions workflow to run on push and pull_request to `master`, so that every change is verified before merge.
16. As a CI engineer, I want the matrix to cover Ubuntu/GCC, Ubuntu/Clang, macOS/AppleClang, and Windows/MSVC, so that oscpp's documented platform support is continuously verified.
17. As a CI engineer, I want FetchContent downloads to be cached between runs keyed on dependency GIT_TAGs, so that CI does not re-clone Catch2 and RapidCheck on every run.
18. As a CI engineer, I want failing tests to print full output, so that failures in CI logs are self-explanatory.
19. As a methcla developer, I want to replace the manual oscpp INTERFACE target workaround with a single `FetchContent_Declare` + `FetchContent_MakeAvailable` block pinned to `1.0.0`, so that oscpp is consumed as a first-class versioned dependency.
20. As a methcla developer, I want the oscpp FetchContent block to not trigger oscpp's tests, so that methcla's build is not affected by oscpp's test dependencies.

## Implementation Decisions

### Modules

**`CMakeLists.txt` (root)**
Defines the `oscpp` INTERFACE target and its alias `oscpp::oscpp`. Propagates include directories and `cxx_std_11` to consumers. Owns the install/export block and generates `oscppConfigVersion.cmake`. Conditionally pulls in the test subdirectory.

- `OSCPP_BUILD_TESTS` defaults to `${PROJECT_IS_TOP_LEVEL}`: on for direct builds, off when consumed via FetchContent or `add_subdirectory`. No CMake preset needs to set this explicitly.
- `oscppConfigVersion.cmake` uses `SameMajorVersion` compatibility, allowing `find_package(oscpp 1.x)` while rejecting `find_package(oscpp 2.0)`.

**`test/CMakeLists.txt`**
Owns all test targets. Fetches Catch2 `v3.8.1` and RapidCheck `b2d9ed2` via FetchContent. Defines two executables: `oscpp_autocheck` (property tests) and `oscpp_readme` (README smoke test). Applies `-Wall -Wextra -Wno-unused-parameter -Werror` to both as `PRIVATE` compile options.

**`CMakePresets.json`**
Two configure presets: `debug` (`CMAKE_BUILD_TYPE=Debug`) and `release` (`CMAKE_BUILD_TYPE=Release`). Build type and test enablement are independent — presets only set build type.

**`tools/mdcode.py`**
Replaces `tools/mdcode.rb`. Extracts only ` ```cpp `-tagged code blocks from a Markdown file and concatenates them into a single `.cpp` file. Invoked by the `oscpp_readme` custom command. Python 3 stdlib only.

**`test/oscpp_autocheck.cpp`** (modified)
Migrated from autocheck to RapidCheck + Catch2. The inline AST helpers remain in the same file. Two properties:
- `prop_identity`: generate a random OSC packet, serialise it, deserialise it, assert equality.
- `prop_overflow`: generate a random OSC packet, attempt to write it into an undersized buffer, assert `OSCPP::OverflowError` is thrown.
`prop_overflow` was previously commented out and is re-enabled. The stray `std::cerr` debug print in `prop_overflow` is removed.

**`.github/workflows/ci.yml`**
Matrix: `ubuntu-latest`/GCC, `ubuntu-latest`/Clang, `macos-latest`/AppleClang, `windows-latest`/MSVC. Steps: checkout, cache FetchContent deps, `cmake --preset debug`, `cmake --build`, `ctest --output-on-failure`. Cache key includes both dependency GIT_TAGs.

### Dependency pinning

| Dependency | Pin |
|---|---|
| Catch2 | `v3.8.1` |
| RapidCheck | `b2d9ed2dddefc4b84318d664b4f221eb792d89c7` (master HEAD, 2026-05-19 — no release tags) |

### README fences

README code blocks migrated from Pandoc-style `~~~~cpp` / `~~~~` to CommonMark backtick fences. `mdcode.py` matches ` ```cpp ` tagged blocks only; untagged blocks (shell commands, example output) are ignored.

### Removals

- `test/autocheck` git submodule delisted and removed
- `tools/mdcode.rb` deleted
- Root `Makefile` deleted
- `.travis.yml` deleted

## Testing Decisions

**What makes a good test here:** tests verify observable external behaviour, not implementation details. For the property tests, the observable behaviour is the OSC wire format round-trip and overflow detection — not the internal structure of the AST or generator. For the smoke test, the observable behaviour is that the README examples compile and produce the documented output.

**`oscpp_autocheck` (property-based tests)**
- `prop_identity`: the only correct oracle is that serialise ∘ deserialise = identity over all valid OSC packets. RapidCheck generates and shrinks; no hand-written cases needed.
- `prop_overflow`: the only correct behaviour for an undersized buffer is `OSCPP::OverflowError`. The property generates packets and buffer sizes independently.
- RapidCheck's Catch2 integration surfaces failures as Catch2 test failures with the shrunk counterexample printed inline.

**`oscpp_readme` (smoke test)**
- Extracted README C++ blocks must compile cleanly and exit 0.
- Verifies that the public API as documented is always valid — catches documentation drift.
- No assertions beyond successful execution; the README examples print output but do not assert.

**CMake integration (consumer test)**
- A minimal throwaway consumer `CMakeLists.txt` that calls `find_package(oscpp 1.0 REQUIRED)` and `target_link_libraries(... PRIVATE oscpp::oscpp)`.
- Verified manually at Checkpoint 2 of the implementation plan (not automated in CI, but documented as a required manual gate before tagging 1.0.0).
- Also verifies FetchContent consumption does not pull in test dependencies (confirmed by checking that `OSCPP_BUILD_TESTS` is `OFF` when `PROJECT_IS_TOP_LEVEL` is false).

## Out of Scope

- Windows CI was previously deferred but is now in scope (added to matrix).
- iOS, Android, and WASM targets.
- Package manager publishing (vcpkg, Conan).
- Doxygen documentation generation changes.
- Any changes to the oscpp public API or wire format.
- methcla CMake work (tracked separately in `docs/modernisation-todo.md`).

## Further Notes

- `oscppConfigVersion.cmake` uses `SameMajorVersion` — a future 2.0 release would be a declared compatibility break.
- RapidCheck has no tagged releases; the pinned SHA should be reviewed when updating. The absence of tags is a known upstream issue.
- `prop_overflow` was previously commented out with no explanation. It is re-enabled on the assumption the comment-out was temporary. If it fails during migration, debug before proceeding — do not re-comment it.
- The ADR for the Python script decision is at `docs/adr/0001-python-for-readme-extraction-script.md`.
- Full implementation steps and checkpoints are in `PLAN.md` at the repo root.
