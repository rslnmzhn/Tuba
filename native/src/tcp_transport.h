#ifndef RC_NATIVE_TCP_TRANSPORT_H_
#define RC_NATIVE_TCP_TRANSPORT_H_

#include "socket_platform.h"
#include "transport.h"

#include <string>

namespace rc::transport {

class TcpTransport final : public ITransport {
 public:
  TcpTransport();
  explicit TcpTransport(rc_socket_t socket_handle);
  ~TcpTransport() override;

  TcpTransport(const TcpTransport&) = delete;
  TcpTransport& operator=(const TcpTransport&) = delete;
  TcpTransport(TcpTransport&& other) noexcept;
  TcpTransport& operator=(TcpTransport&& other) noexcept;

  bool connect(const char* host, uint16_t port) override;
  int send(const uint8_t* data, std::size_t length) override;
  int recv(uint8_t* buffer, std::size_t length) override;
  void disconnect() override;

  rc_socket_t socket_handle() const;
  std::string peer_address() const;

  static rc_socket_t listen_on(uint16_t port, int backlog);
  static TcpTransport accept_from(rc_socket_t listener);

 private:
  rc_socket_t socket_handle_;
};

const char* last_tcp_error();

}  // namespace rc::transport

#endif  // RC_NATIVE_TCP_TRANSPORT_H_
