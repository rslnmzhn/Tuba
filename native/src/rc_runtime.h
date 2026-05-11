#ifndef RC_NATIVE_RC_RUNTIME_H_
#define RC_NATIVE_RC_RUNTIME_H_

#include <cstdint>
#include <memory>

namespace rc::transport {
class TlsTransport;
class UdpDiscoveryResponder;
}

namespace rc::capture {
class ICapture;
}

namespace rc {

class Runtime final {
 public:
  Runtime();
  ~Runtime();

  Runtime(const Runtime&) = delete;
  Runtime& operator=(const Runtime&) = delete;

  int32_t start_server(uint16_t port, const uint8_t* psk,
                       uint32_t psk_length);
  int32_t connect_client(const char* ip_address, uint16_t port,
                         const uint8_t* psk, uint32_t psk_length);
  int32_t start_discovery(uint16_t discovery_port, const char* device_name,
                          uint16_t tcp_port);
  int32_t start_capture();
  void stop_capture();

 private:
  std::unique_ptr<transport::TlsTransport> server_session_;
  std::unique_ptr<transport::TlsTransport> client_session_;
  std::unique_ptr<transport::UdpDiscoveryResponder> discovery_responder_;
  std::unique_ptr<capture::ICapture> capture_;
};

Runtime& runtime();

}  // namespace rc

#endif  // RC_NATIVE_RC_RUNTIME_H_
