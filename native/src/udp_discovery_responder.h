#ifndef RC_NATIVE_UDP_DISCOVERY_RESPONDER_H_
#define RC_NATIVE_UDP_DISCOVERY_RESPONDER_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace rc::transport {

class UdpDiscoveryResponder final {
 public:
  UdpDiscoveryResponder();
  ~UdpDiscoveryResponder();

  UdpDiscoveryResponder(const UdpDiscoveryResponder&) = delete;
  UdpDiscoveryResponder& operator=(const UdpDiscoveryResponder&) = delete;

  bool start(uint16_t port, const char* device_name, uint16_t tcp_port);
  void stop();

 private:
  void run(uint16_t port, std::string device_name, uint16_t tcp_port);

  std::atomic_bool running_;
  std::thread thread_;
};

}  // namespace rc::transport

#endif  // RC_NATIVE_UDP_DISCOVERY_RESPONDER_H_
