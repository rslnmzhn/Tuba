#ifndef RC_NATIVE_UDP_DISCOVERY_H_
#define RC_NATIVE_UDP_DISCOVERY_H_

#include <cstdint>

namespace rc::transport {

class UdpDiscovery final {
 public:
  static bool announce_once(uint16_t port, const char* device_name,
                            uint16_t tcp_port);
  static bool query_once(uint16_t port, uint32_t timeout_ms);
};

}  // namespace rc::transport

#endif  // RC_NATIVE_UDP_DISCOVERY_H_
