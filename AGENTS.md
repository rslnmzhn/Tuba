# AGENTS.md

## Project
name: Tuba
build: flutter clean && flutter build apk; flutter build windows
test: flutter test
lint: flutter analyze
format: dart format .

## Architecture rules
- Native public API must remain pure C extern "C" with primitive C-compatible types only.
- C ABI facade rc_api.cpp must stay thin: validation and delegation only, no orchestration ownership.
- Runtime must own server_session_, client_session_, discovery_responder_, active session transports, and capture_.
- Dart FFI loader lives under lib/ffi and must choose library name/path by Platform.
- Native build ownership remains in native/ with platform build files only wiring CMake target.
- C++ shared library public declarations live in native/include/rc_api.h; implementation lives in native/src/rc_api.cpp.
- CMake target rc_native is integrated through Android, Linux, and Windows platform build files.
- Codec layer must stay isolated from transport and Runtime ownership.

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
- Transport scaffold includes ITransport, TcpTransport, TlsTransport with mbedTLS PSK, UDP discovery/responder, and NatTransport stub behind C ABI exports.
- C ABI facade rc_api.cpp should remain thin validation/default-port/delegation only; Runtime owns sessions, discovery responder, and transport construction.
- Runtime must retain active session transports, not just construct local TlsTransport objects.
- Capture ownership belongs to Runtime via capture_; rc_api.cpp remains only validation/delegation.
- Platform capture implementations live in separate .cpp files selected by native/CMakeLists.txt to avoid #ifdef sprawl in shared code.
- rc_api.cpp should include only rc_api.h and rc_runtime.h for transport API delegation.
- Dart FFI loader lives under lib/ffi and must choose library name/path by Platform.
- Native build ownership remains in native/ with platform build files only wiring CMake target.
- Codec layer is isolated from transport and Runtime ownership.
- H264/OpenH264/libyuv wiring currently lives in native/CMakeLists.txt and may need extraction if dependency setup grows.
- Android CMake cache may require flutter clean when toggling RC_NATIVE_ENABLE_CODEC.
- Verified commands: dart format ., flutter analyze, flutter build windows, flutter clean && flutter build apk, flutter test.

## Blocked items
(items that require human decision before automated work can proceed)
- flutter build linux is unsupported on the current Windows host.

## Changelog
- 2026-05-12: Documented step 2 codec abstraction, H264 dependency wiring, and Android codec cache-clean note.
- 2026-05-12: Documented step 1 capture scaffold ownership and platform file selection after Windows DXGI/stub capture implementation.
- 2026-05-12: Updated Runtime ownership and rc_api.cpp delegation/include patterns after orchestration refactor passed review.
- 2026-05-12: Documented transport scaffold ownership rule after review found rc_api.cpp still owning orchestration.
- 2026-05-11: Created AGENTS.md documenting Flutter/C++ FFI scaffold, native build ownership, and verified commands.
