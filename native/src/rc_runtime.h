#ifndef RC_NATIVE_RC_RUNTIME_H_
#define RC_NATIVE_RC_RUNTIME_H_

#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>

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

  int32_t initialize_dart_api(void* initialize_api_data);
  int32_t start_server(uint16_t port, const uint8_t* psk,
                       uint32_t psk_length);
  int32_t connect_client(const char* ip_address, uint16_t port,
                          const uint8_t* psk, uint32_t psk_length);
  void disconnect_client();
  int32_t start_discovery(uint16_t discovery_port, const char* device_name,
                           uint16_t tcp_port);
  int32_t start_capture();
  void stop_capture();
  int32_t start_frame_stream(int64_t native_port);
  void stop_frame_stream();
  int32_t send_mouse_move(double x, double y);
  int32_t send_mouse_down(double x, double y, int32_t button);
  int32_t send_mouse_up(double x, double y, int32_t button);

 private:
  void frame_stream_loop(int64_t native_port);

  std::unique_ptr<transport::TlsTransport> server_session_;
  std::unique_ptr<transport::TlsTransport> client_session_;
  std::unique_ptr<transport::UdpDiscoveryResponder> discovery_responder_;
  std::unique_ptr<capture::ICapture> capture_;
  std::atomic_bool frame_stream_running_{false};
  std::thread frame_stream_thread_;
};

Runtime& runtime();

}  // namespace rc

#endif  // RC_NATIVE_RC_RUNTIME_H_
