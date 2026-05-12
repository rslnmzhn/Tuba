#include "socket_platform.h"

#include <array>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#endif

#if !defined(_WIN32)
#include <cerrno>
#include <unistd.h>
#endif

namespace rc::transport {

bool initialize_sockets() {
#if defined(_WIN32)
  WSADATA data{};
  return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
  return true;
#endif
}

void shutdown_sockets() {
#if defined(_WIN32)
  WSACleanup();
#endif
}

void close_socket(rc_socket_t socket_handle) {
  if (socket_handle == kInvalidSocket) {
    return;
  }
#if defined(_WIN32)
  closesocket(socket_handle);
#else
  close(socket_handle);
#endif
}

bool set_reuse_addr(rc_socket_t socket_handle) {
  int enabled = 1;
  return setsockopt(socket_handle, SOL_SOCKET, SO_REUSEADDR,
                    reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == 0;
}

bool set_broadcast(rc_socket_t socket_handle) {
  int enabled = 1;
  return setsockopt(socket_handle, SOL_SOCKET, SO_BROADCAST,
                    reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == 0;
}

bool set_recv_timeout(rc_socket_t socket_handle, uint32_t timeout_ms) {
#if defined(_WIN32)
  DWORD timeout = timeout_ms;
  return setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0;
#else
  timeval timeout{};
  timeout.tv_sec = static_cast<time_t>(timeout_ms / 1000U);
  timeout.tv_usec = static_cast<suseconds_t>((timeout_ms % 1000U) * 1000U);
  return setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                    sizeof(timeout)) == 0;
#endif
}

bool socket_would_block() {
#if defined(_WIN32)
  const int error = WSAGetLastError();
  return error == WSAEWOULDBLOCK || error == WSAETIMEDOUT;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

int last_socket_error() {
#if defined(_WIN32)
  return WSAGetLastError();
#else
  return errno;
#endif
}

const char* socket_error_message(int error_code) {
  static thread_local std::array<char, 256> buffer{};
#if defined(_WIN32)
  const DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD length = FormatMessageA(
      flags, nullptr, static_cast<DWORD>(error_code),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer.data(),
      static_cast<DWORD>(buffer.size()), nullptr);
  if (length == 0U) {
    std::snprintf(buffer.data(), buffer.size(), "socket error %d", error_code);
  }
#else
  const char* message = std::strerror(error_code);
  std::snprintf(buffer.data(), buffer.size(), "%s", message != nullptr ? message : "unknown socket error");
#endif
  return buffer.data();
}

}  // namespace rc::transport
