#ifndef RC_NATIVE_TLS_TRANSPORT_H_
#define RC_NATIVE_TLS_TRANSPORT_H_

#include "tcp_transport.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>

#include <cstdint>
#include <vector>

namespace rc::transport {

enum class TlsMode { kClient, kServer };

class TlsTransport final : public ITransport {
 public:
  TlsTransport(TcpTransport tcp, TlsMode mode, const uint8_t* psk,
               std::size_t psk_length);
  ~TlsTransport() override;

  TlsTransport(const TlsTransport&) = delete;
  TlsTransport& operator=(const TlsTransport&) = delete;

  bool connect(const char* host, uint16_t port) override;
  bool handshake();
  int send(const uint8_t* data, std::size_t length) override;
  int recv(uint8_t* buffer, std::size_t length) override;
  void disconnect() override;

 private:
  bool configure();

  TcpTransport tcp_;
  TlsMode mode_;
  std::vector<uint8_t> psk_;
  mbedtls_ssl_context ssl_;
  mbedtls_ssl_config config_;
  mbedtls_ctr_drbg_context ctr_drbg_;
  mbedtls_entropy_context entropy_;
  bool configured_;
};

}  // namespace rc::transport

#endif  // RC_NATIVE_TLS_TRANSPORT_H_
