#include "tcp_transport.h"

#include <cstdio>
#include <cstring>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace rc::transport {

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
    return false;
  }

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  char port_text[8]{};
  std::snprintf(port_text, sizeof(port_text), "%u", static_cast<unsigned>(port));

  addrinfo* result = nullptr;
  if (getaddrinfo(host, port_text, &hints, &result) != 0) {
    return false;
  }

  bool connected = false;
  for (addrinfo* item = result; item != nullptr; item = item->ai_next) {
    rc_socket_t candidate = socket(item->ai_family, item->ai_socktype,
                                   item->ai_protocol);
    if (candidate == kInvalidSocket) {
      continue;
    }
    if (::connect(candidate, item->ai_addr,
                  static_cast<int>(item->ai_addrlen)) == 0) {
      socket_handle_ = candidate;
      connected = true;
      break;
    }
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

rc_socket_t TcpTransport::listen_on(uint16_t port, int backlog) {
  if (!initialize_sockets()) {
    return kInvalidSocket;
  }

  rc_socket_t listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener == kInvalidSocket) {
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
  return TcpTransport(accepted);
}

}  // namespace rc::transport
