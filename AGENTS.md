# AGENTS.md

## Project
name: Tuba
build: flutter build apk; flutter build windows
test: flutter test
lint: flutter analyze
format: dart format .

## Architecture rules
- Native public API must remain pure C extern "C" with primitive C-compatible types only.
- Dart FFI loader lives under lib/ffi and must choose library name/path by Platform.
- Native build ownership remains in native/ with platform build files only wiring CMake target.
- C++ shared library public declarations live in native/include/rc_api.h; implementation lives in native/src/rc_api.cpp.
- CMake target rc_native is integrated through Android, Linux, and Windows platform build files.

## Agent registry
| agent | mode | role |
|-------|------|------|
| lite-builder | primary | focused feature/fix/refactor with review loop |
| god-builder  | primary | full-suite build with all scanners |
| context-assembler | subagent | git + project context snapshot |
| terminal-runner   | subagent | safe command execution |
| test-runner       | subagent | test execution and structured reporting |
| quick-reviewer    | subagent | anti-pattern review, 3-attempt loop |
| skill-finder      | subagent | match tasks to available skills |
| agents-keeper     | subagent | AGENTS.md maintenance |
| dead-code-scanner | subagent | unused exports and functions |
| security-scanner  | subagent | secrets and unsafe patterns |
| dependency-auditor| subagent | outdated and vulnerable dependencies |
| complexity-checker| subagent | cyclomatic complexity and nesting hotspots |

## Known patterns
- Baseline app is a Flutter scaffold with a C++ shared library loaded through dart:ffi.
- Native public API must remain pure C extern "C" with primitive C-compatible types only.
- Dart FFI loader lives under lib/ffi and must choose library name/path by Platform.
- Native build ownership remains in native/ with platform build files only wiring CMake target.
- Verified commands: dart format ., flutter analyze, flutter build apk, flutter build windows, flutter test.

## Blocked items
(items that require human decision before automated work can proceed)
- flutter build linux is unsupported on the current Windows host.

## Changelog
- 2026-05-11: Created AGENTS.md documenting Flutter/C++ FFI scaffold, native build ownership, and verified commands.
