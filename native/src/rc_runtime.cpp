#include "rc_runtime.h"

#include "socket_platform.h"
#include "tcp_transport.h"
#include "tls_transport.h"
#include "udp_discovery_responder.h"

#include <utility>

namespace rc {
namespace {

constexpr int kListenBacklog = 1;

}  // namespace

Runtime::Runtime()
    : discovery_responder_(
          std::make_unique<transport::UdpDiscoveryResponder>()) {}

Runtime::~Runtime() = default;

int32_t Runtime::start_server(uint16_t port, const uint8_t* psk,
                              uint32_t psk_length) {
  const rc_socket_t listener =
      transport::TcpTransport::listen_on(port, kListenBacklog);
  if (listener == kInvalidSocket) {
    return -2;
  }

  transport::TcpTransport accepted =
      transport::TcpTransport::accept_from(listener);
  transport::close_socket(listener);
  if (accepted.socket_handle() == kInvalidSocket) {
    return -3;
  }

  auto session = std::make_unique<transport::TlsTransport>(
      std::move(accepted), transport::TlsMode::kServer, psk, psk_length);
  if (!session->handshake()) {
    return -4;
  }
  server_session_ = std::move(session);
  return 0;
}

int32_t Runtime::connect_client(const char* ip_address, uint16_t port,
                                const uint8_t* psk, uint32_t psk_length) {
  auto session = std::make_unique<transport::TlsTransport>(
      transport::TcpTransport(), transport::TlsMode::kClient, psk, psk_length);
  if (!session->connect(ip_address, port)) {
    return -2;
  }
  client_session_ = std::move(session);
  return 0;
}

int32_t Runtime::start_discovery(uint16_t discovery_port,
                                 const char* device_name, uint16_t tcp_port) {
  return discovery_responder_->start(discovery_port, device_name, tcp_port) ? 0
                                                                           : -1;
}

Runtime& runtime() {
  static Runtime instance;
  return instance;
}

}  // namespace rc
