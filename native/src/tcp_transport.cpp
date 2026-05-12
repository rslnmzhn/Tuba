#include "tcp_transport.h"

#include <cstdio>
#include <cstring>
#include <string>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#else
#include <ws2tcpip.h>
#endif

namespace rc::transport {
namespace {

thread_local std::string g_last_tcp_error;

void set_tcp_error(const char* operation, int error_code) {
  g_last_tcp_error = operation;
  g_last_tcp_error += " failed: ";
  g_last_tcp_error += socket_error_message(error_code);
}

}  // namespace

TcpTransport::TcpTransport() : socket_handle_(kInvalidSocket) {}

TcpTransport::TcpTransport(rc_socket_t socket_handle)
    : socket_handle_(socket_handle) {}

TcpTransport::~TcpTransport() {
  disconnect();
}

TcpTransport::TcpTransport(TcpTransport&& other) noexcept
    : socket_handle_(other.socket_handle_) {
  other.socket_handle_ = kInvalidSocket;
}

TcpTransport& TcpTransport::operator=(TcpTransport&& other) noexcept {
  if (this != &other) {
    disconnect();
    socket_handle_ = other.socket_handle_;
    other.socket_handle_ = kInvalidSocket;
  }
  return *this;
}

bool TcpTransport::connect(const char* host, uint16_t port) {
  disconnect();
  if (host == nullptr || !initialize_sockets()) {
    g_last_tcp_error = "socket initialization failed";
    return false;
  }

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  char port_text[8]{};
  std::snprintf(port_text, sizeof(port_text), "%u", static_cast<unsigned>(port));

  addrinfo* result = nullptr;
  const int resolve_result = getaddrinfo(host, port_text, &hints, &result);
  if (resolve_result != 0) {
    g_last_tcp_error = "getaddrinfo failed";
    return false;
  }

  bool connected = false;
  for (addrinfo* item = result; item != nullptr; item = item->ai_next) {
    rc_socket_t candidate = socket(item->ai_family, item->ai_socktype,
                                   item->ai_protocol);
    if (candidate == kInvalidSocket) {
      set_tcp_error("socket", last_socket_error());
      continue;
    }
    if (::connect(candidate, item->ai_addr,
                  static_cast<int>(item->ai_addrlen)) == 0) {
      socket_handle_ = candidate;
      connected = true;
      g_last_tcp_error.clear();
      break;
    }
    set_tcp_error("connect", last_socket_error());
    close_socket(candidate);
  }

  freeaddrinfo(result);
  return connected;
}

int TcpTransport::send(const uint8_t* data, std::size_t length) {
  if (socket_handle_ == kInvalidSocket || data == nullptr || length == 0U) {
    return -1;
  }
  const int sent = ::send(socket_handle_, reinterpret_cast<const char*>(data),
                          static_cast<int>(length), 0);
  return sent > 0 ? sent : -1;
}

int TcpTransport::recv(uint8_t* buffer, std::size_t length) {
  if (socket_handle_ == kInvalidSocket || buffer == nullptr || length == 0U) {
    return -1;
  }
  const int received = ::recv(socket_handle_, reinterpret_cast<char*>(buffer),
                              static_cast<int>(length), 0);
  return received > 0 ? received : -1;
}

void TcpTransport::disconnect() {
  close_socket(socket_handle_);
  socket_handle_ = kInvalidSocket;
}

rc_socket_t TcpTransport::socket_handle() const {
  return socket_handle_;
}

std::string TcpTransport::peer_address() const {
  if (socket_handle_ == kInvalidSocket) {
    return {};
  }
  sockaddr_storage address{};
  socklen_t address_length = sizeof(address);
  if (getpeername(socket_handle_, reinterpret_cast<sockaddr*>(&address),
                  &address_length) != 0) {
    return {};
  }
  char text[INET6_ADDRSTRLEN]{};
  if (address.ss_family == AF_INET) {
    const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(&address);
    inet_ntop(AF_INET, &ipv4->sin_addr, text, sizeof(text));
  } else if (address.ss_family == AF_INET6) {
    const auto* ipv6 = reinterpret_cast<const sockaddr_in6*>(&address);
    inet_ntop(AF_INET6, &ipv6->sin6_addr, text, sizeof(text));
  }
  return text;
}

rc_socket_t TcpTransport::listen_on(uint16_t port, int backlog) {
  if (!initialize_sockets()) {
    g_last_tcp_error = "socket initialization failed";
    return kInvalidSocket;
  }

  rc_socket_t listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener == kInvalidSocket) {
    set_tcp_error("socket", last_socket_error());
    return kInvalidSocket;
  }

  set_reuse_addr(listener);

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
          0 ||
      listen(listener, backlog) != 0) {
    set_tcp_error("bind/listen", last_socket_error());
    close_socket(listener);
    return kInvalidSocket;
  }

  return listener;
}

TcpTransport TcpTransport::accept_from(rc_socket_t listener) {
  if (listener == kInvalidSocket) {
    return TcpTransport();
  }
  sockaddr_storage address{};
  socklen_t address_length = sizeof(address);
  rc_socket_t accepted = accept(listener, reinterpret_cast<sockaddr*>(&address),
                                &address_length);
  if (accepted == kInvalidSocket) {
    set_tcp_error("accept", last_socket_error());
  }
  return TcpTransport(accepted);
}

const char* last_tcp_error() {
  return g_last_tcp_error.c_str();
}

}  // namespace rc::transport
