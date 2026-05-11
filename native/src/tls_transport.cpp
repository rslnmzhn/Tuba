#include "tls_transport.h"

#include <cstring>
#include <utility>

namespace rc::transport {
namespace {

constexpr char kPskIdentity[] = "tuba-psk-v1";
constexpr char kDrbgPersonalization[] = "tuba-transport";

int tls_send_callback(void* context, const unsigned char* buffer,
                      size_t length) {
  auto* tcp = static_cast<TcpTransport*>(context);
  const int result = tcp->send(buffer, length);
  return result < 0 ? MBEDTLS_ERR_SSL_WANT_WRITE : result;
}

int tls_recv_callback(void* context, unsigned char* buffer, size_t length) {
  auto* tcp = static_cast<TcpTransport*>(context);
  const int result = tcp->recv(buffer, length);
  return result < 0 ? MBEDTLS_ERR_SSL_WANT_READ : result;
}

}  // namespace

TlsTransport::TlsTransport(TcpTransport tcp, TlsMode mode, const uint8_t* psk,
                           std::size_t psk_length)
    : tcp_(std::move(tcp)),
      mode_(mode),
      psk_(psk, psk + psk_length),
      configured_(false) {
  mbedtls_ssl_init(&ssl_);
  mbedtls_ssl_config_init(&config_);
  mbedtls_ctr_drbg_init(&ctr_drbg_);
  mbedtls_entropy_init(&entropy_);
}

TlsTransport::~TlsTransport() {
  disconnect();
  mbedtls_ssl_free(&ssl_);
  mbedtls_ssl_config_free(&config_);
  mbedtls_ctr_drbg_free(&ctr_drbg_);
  mbedtls_entropy_free(&entropy_);
}

bool TlsTransport::connect(const char* host, uint16_t port) {
  if (mode_ != TlsMode::kClient || !tcp_.connect(host, port)) {
    return false;
  }
  return handshake();
}

bool TlsTransport::handshake() {
  if (!configure()) {
    return false;
  }
  int result = 0;
  do {
    result = mbedtls_ssl_handshake(&ssl_);
  } while (result == MBEDTLS_ERR_SSL_WANT_READ ||
           result == MBEDTLS_ERR_SSL_WANT_WRITE);
  return result == 0;
}

int TlsTransport::send(const uint8_t* data, std::size_t length) {
  if (data == nullptr || length == 0U) {
    return -1;
  }
  int result = 0;
  do {
    result = mbedtls_ssl_write(&ssl_, data, length);
  } while (result == MBEDTLS_ERR_SSL_WANT_READ ||
           result == MBEDTLS_ERR_SSL_WANT_WRITE);
  return result > 0 ? result : -1;
}

int TlsTransport::recv(uint8_t* buffer, std::size_t length) {
  if (buffer == nullptr || length == 0U) {
    return -1;
  }
  int result = 0;
  do {
    result = mbedtls_ssl_read(&ssl_, buffer, length);
  } while (result == MBEDTLS_ERR_SSL_WANT_READ ||
           result == MBEDTLS_ERR_SSL_WANT_WRITE);
  return result > 0 ? result : -1;
}

void TlsTransport::disconnect() {
  if (configured_) {
    mbedtls_ssl_close_notify(&ssl_);
  }
  tcp_.disconnect();
  configured_ = false;
}

bool TlsTransport::configure() {
  if (configured_ || psk_.empty()) {
    return configured_;
  }

  const int endpoint = mode_ == TlsMode::kClient ? MBEDTLS_SSL_IS_CLIENT
                                                 : MBEDTLS_SSL_IS_SERVER;
  if (mbedtls_ctr_drbg_seed(
          &ctr_drbg_, mbedtls_entropy_func, &entropy_,
          reinterpret_cast<const unsigned char*>(kDrbgPersonalization),
          std::strlen(kDrbgPersonalization)) != 0) {
    return false;
  }
  if (mbedtls_ssl_config_defaults(&config_, endpoint, MBEDTLS_SSL_TRANSPORT_STREAM,
                                  MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
    return false;
  }

  mbedtls_ssl_conf_rng(&config_, mbedtls_ctr_drbg_random, &ctr_drbg_);
  mbedtls_ssl_conf_authmode(&config_, MBEDTLS_SSL_VERIFY_REQUIRED);
  if (mbedtls_ssl_conf_psk(
          &config_, psk_.data(), psk_.size(),
          reinterpret_cast<const unsigned char*>(kPskIdentity),
          std::strlen(kPskIdentity)) != 0) {
    return false;
  }

  if (mbedtls_ssl_setup(&ssl_, &config_) != 0) {
    return false;
  }
  mbedtls_ssl_set_bio(&ssl_, &tcp_, tls_send_callback, tls_recv_callback,
                      nullptr);
  configured_ = true;
  return true;
}

}  // namespace rc::transport
