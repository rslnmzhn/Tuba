#include "rc_api.h"

#include "rc_runtime.h"

#include <cstdint>

namespace {

constexpr uint16_t kDefaultTcpPort = 5900;
constexpr uint16_t kDefaultDiscoveryPort = 5901;

bool valid_psk(const uint8_t* psk, uint32_t psk_length) {
  return psk != nullptr && psk_length > 0U;
}

}  // namespace

extern "C" int32_t rc_native_abi_version(void) {
  return 1;
}

extern "C" int32_t rc_server_start(uint16_t port, const uint8_t* psk,
                                    uint32_t psk_length) {
  if (!valid_psk(psk, psk_length)) {
    return -1;
  }
  const uint16_t listen_port = port == 0U ? kDefaultTcpPort : port;
  return rc::runtime().start_server(listen_port, psk, psk_length);
}

extern "C" int32_t rc_client_connect(const char* ip_address, uint16_t port,
                                      const uint8_t* psk,
                                      uint32_t psk_length) {
  if (ip_address == nullptr || !valid_psk(psk, psk_length)) {
    return -1;
  }
  const uint16_t connect_port = port == 0U ? kDefaultTcpPort : port;
  return rc::runtime().connect_client(ip_address, connect_port, psk, psk_length);
}

extern "C" int32_t rc_discovery_start(uint16_t port, const char* device_name) {
  const uint16_t discovery_port = port == 0U ? kDefaultDiscoveryPort : port;
  return rc::runtime().start_discovery(discovery_port, device_name,
                                       kDefaultTcpPort);
}
