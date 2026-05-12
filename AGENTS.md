# AGENTS.md

## Project
name: Tuba
build: flutter clean && flutter build apk; flutter build windows
test: flutter test
lint: flutter analyze
format: dart format .

## Architecture rules
- Native public API must remain pure C extern "C" with primitive C-compatible types only.
- Native ABI errors must be exposed through rc_last_error rather than exceptions or non-C-compatible returns.
- C ABI facade rc_api.cpp must stay thin: validation and delegation only, no orchestration ownership.
- Runtime must own server_session_, client_session_, discovery_responder_, active session transports, and capture_.
- Dart FFI loader lives under lib/ffi and must choose library name/path by Platform.
- NativePort frame streams must initialize Dart DL before starting frame delivery.
- NativePort is the standard pattern for async approval, discovery, and frame events from native code to Dart.
- Native build ownership remains in native/ with platform build files only wiring CMake target.
- C++ shared library public declarations live in native/include/rc_api.h; implementation lives in native/src/rc_api.cpp.
- CMake target rc_native is integrated through Android, Linux, and Windows platform build files.
- Codec layer must stay isolated from transport and Runtime ownership.

## Project structure
- `lib/ffi`: Dart FFI loading and bridge code; choose native library name/path by Platform.
- `native/include/rc_api.h`: Public C-compatible native API declarations.
- `native/src/rc_api.cpp`: Thin C ABI validation/delegation facade.
- `native/src`: Runtime, transport, capture, codec, and native implementation internals.
- `native/CMakeLists.txt`: Native rc_native target and dependency wiring.
- `android`, `linux`, `windows`: Flutter platform integration that wires the native CMake target.
- `/skills`, `/docs/build`, `/docs/context`, `/docs/deadcode`, `/docs/deps`, `/docs/security`: Local agent artifacts only; keep out of tracked git history.

## Business rules
- TLS session approval is the first application message after transport establishment; rejected clients receive result code -3.

## Code conventions
- Generated agent reports and downloaded skills are local artifacts; do not commit them.
- HomeScreen must not own discovered-device state; use the DiscoveredDevices service.

## Agent registry
| agent | mode | role |
|-------|------|------|
| lite-builder | primary | focused feature/fix/refactor with review loop |
| god-builder  | primary | full-suite build with all scanners |
| context-assembler | subagent | git + project context snapshot |
| terminal-runner   | subagent | safe command execution |
| test-runner       | subagent | test execution and structured reporting |
| quick-reviewer    | subagent | anti-pattern review, 3-attempt loop |
| skill-finder      | subagent | remote skill search and local install into ./skills |
| agents-keeper     | subagent | AGENTS.md maintenance |
| dead-code-scanner | subagent | unused exports and functions |
| security-scanner  | subagent | secrets and unsafe patterns |
| dependency-auditor| subagent | outdated and vulnerable dependencies |
| complexity-checker| subagent | cyclomatic complexity and nesting hotspots |

## Known patterns
- Baseline app is a Flutter scaffold with a C++ shared library loaded through dart:ffi.
- Native public API must remain pure C extern "C" with primitive C-compatible types only.
- Transport scaffold includes ITransport, TcpTransport, TlsTransport with mbedTLS PSK, UDP discovery/responder, and NatTransport stub behind C ABI exports.
- Approval protocol runs over TLS as the first message before normal session traffic.
- Client rejection uses stable result code -3.
- rc_last_error is the stable C ABI surface for native error details.
- NativePort event callbacks are used for approval and discovery events as well as frame streams.
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
- NativePort frame delivery must initialize Dart DL via rc_initialize_dart_api(NativeApi.initializeApiDLData) before rc_frame_stream_start.
- HomeScreen must not own connection history persistence; use the ConnectionHistory service.
- HomeScreen must not own discovered-device state; use the DiscoveredDevices service.
- RcBridge/Runtime are growing seams and should be split if more FFI domains are added.
- Generated agent context/build/deadcode/deps/security reports and downloaded skills are local artifacts and should stay out of tracked git history.
- Verified commands: dart format ., flutter analyze, flutter build windows, flutter clean && flutter build apk, flutter test.

## Blocked items
(items that require human decision before automated work can proceed)
- flutter build linux is unsupported on the current Windows host.
