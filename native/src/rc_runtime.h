#ifndef RC_NATIVE_RC_RUNTIME_H_
#define RC_NATIVE_RC_RUNTIME_H_

#include "socket_platform.h"

#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

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
  int32_t start_server_async(uint16_t port, const uint8_t* psk,
                             uint32_t psk_length);
  int32_t set_approval_port(int64_t native_port);
  int32_t approve_pending(int32_t request_id);
  int32_t reject_pending(int32_t request_id);
  int32_t connect_client(const char* ip_address, uint16_t port,
                           const uint8_t* psk, uint32_t psk_length);
  int32_t connect_client(const char* ip_address, uint16_t port,
                         const uint8_t* psk, uint32_t psk_length,
                         const char* device_name);
  void disconnect_client();
  int32_t start_discovery(uint16_t discovery_port, const char* device_name,
                            uint16_t tcp_port);
  int32_t query_discovery(int64_t native_port, uint16_t discovery_port,
                          uint32_t timeout_ms);
  const char* last_error() const;
  int32_t start_capture();
  void stop_capture();
  int32_t start_frame_stream(int64_t native_port);
  void stop_frame_stream();
  int32_t send_mouse_move(double x, double y);
  int32_t send_mouse_down(double x, double y, int32_t button);
  int32_t send_mouse_up(double x, double y, int32_t button);

 private:
  enum class ApprovalDecision { kPending, kApproved, kRejected };

  int32_t wait_for_approval(const std::string& device_name,
                            const std::string& ip_address);
  void set_last_error(std::string message);
  void frame_stream_loop(int64_t native_port);
  void stop_server_thread();

  std::unique_ptr<transport::TlsTransport> server_session_;
  std::unique_ptr<transport::TlsTransport> client_session_;
  std::unique_ptr<transport::UdpDiscoveryResponder> discovery_responder_;
  std::unique_ptr<capture::ICapture> capture_;
  mutable std::mutex state_mutex_;
  std::mutex server_listener_mutex_;
  std::condition_variable approval_cv_;
  std::string last_error_;
  int64_t approval_port_{0};
  int32_t next_request_id_{1};
  int32_t pending_request_id_{0};
  ApprovalDecision approval_decision_{ApprovalDecision::kPending};
  std::atomic_bool server_running_{false};
  rc_socket_t server_listener_{kInvalidSocket};
  std::thread server_thread_;
  std::atomic_bool frame_stream_running_{false};
  std::thread frame_stream_thread_;
};

Runtime& runtime();

}  // namespace rc

#endif  // RC_NATIVE_RC_RUNTIME_H_
