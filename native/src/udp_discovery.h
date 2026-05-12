#ifndef RC_NATIVE_UDP_DISCOVERY_H_
#define RC_NATIVE_UDP_DISCOVERY_H_

#include <cstdint>
#include <string>
#include <vector>

namespace rc::transport {

struct DiscoveredDevice final {
  std::string name;
  std::string ip_address;
  uint16_t tcp_port;
};

class UdpDiscovery final {
 public:
  static bool announce_once(uint16_t port, const char* device_name,
                            uint16_t tcp_port);
  static bool query_once(uint16_t port, uint32_t timeout_ms);
  static std::vector<DiscoveredDevice> query(uint16_t port,
                                             uint32_t timeout_ms);
};

}  // namespace rc::transport

#endif  // RC_NATIVE_UDP_DISCOVERY_H_
