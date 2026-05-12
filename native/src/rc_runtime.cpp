#include "rc_runtime.h"

#include "approval_protocol.h"
#include "capture_factory.h"
#include "dart_port_events.h"
#include "socket_platform.h"
#include "tcp_transport.h"
#include "tls_transport.h"
#include "udp_discovery.h"
#include "udp_discovery_responder.h"

#include <chrono>
#include <cstdio>
#include <utility>
#include <vector>

#if __has_include("dart_api_dl.h")
#include "dart_api_dl.h"
#define RC_HAS_DART_API_DL 1
#elif __has_include(<dart_api_dl.h>)
#include <dart_api_dl.h>
#define RC_HAS_DART_API_DL 1
#else
#define RC_HAS_DART_API_DL 0
#endif

namespace rc {
namespace {

constexpr int kListenBacklog = 1;
constexpr auto kFrameInterval = std::chrono::milliseconds(33);
constexpr auto kApprovalTimeout = std::chrono::seconds(60);
constexpr char kApproved[] = "APPROVED";
constexpr char kRejected[] = "REJECTED";

#if RC_HAS_DART_API_DL
void post_frame_to_dart(int64_t native_port,
                        const capture::FrameBuffer& frame) {
  if (frame.empty()) {
    return;
  }
  if (frame.format != capture::PixelFormat::kRgba8888 &&
      frame.format != capture::PixelFormat::kBgra8888) {
    return;
  }

  Dart_CObject width;
  width.type = Dart_CObject_kInt32;
  width.value.as_int32 = static_cast<int32_t>(frame.width);

  Dart_CObject height;
  height.type = Dart_CObject_kInt32;
  height.value.as_int32 = static_cast<int32_t>(frame.height);

  Dart_CObject format;
  format.type = Dart_CObject_kInt32;
  format.value.as_int32 =
      frame.format == capture::PixelFormat::kRgba8888 ? 2 : 1;

  Dart_CObject pixels;
  pixels.type = Dart_CObject_kTypedData;
  pixels.value.as_typed_data.type = Dart_TypedData_kUint8;
  pixels.value.as_typed_data.length = frame.pixels.size();
  pixels.value.as_typed_data.values =
      const_cast<uint8_t*>(frame.pixels.data());

  Dart_CObject* values[] = {&width, &height, &format, &pixels};
  Dart_CObject message;
  message.type = Dart_CObject_kArray;
  message.value.as_array.length = 4;
  message.value.as_array.values = values;

  Dart_PostCObject_DL(native_port, &message);
}
#else
void post_frame_to_dart(int64_t native_port,
                        const capture::FrameBuffer& frame) {
  (void)native_port;
  (void)frame;
}
#endif

}  // namespace

Runtime::Runtime()
    : discovery_responder_(
          std::make_unique<transport::UdpDiscoveryResponder>()) {}

Runtime::~Runtime() {
  stop_server_thread();
}

int32_t Runtime::initialize_dart_api(void* initialize_api_data) {
  return initialize_dart_api_dl(initialize_api_data);
}

int32_t Runtime::start_server(uint16_t port, const uint8_t* psk,
                              uint32_t psk_length) {
  const rc_socket_t listener =
      transport::TcpTransport::listen_on(port, kListenBacklog);
  if (listener == kInvalidSocket) {
    set_last_error(transport::last_tcp_error());
    return -2;
  }
  {
    std::lock_guard<std::mutex> lock(server_listener_mutex_);
    server_listener_ = listener;
  }

  transport::TcpTransport accepted =
      transport::TcpTransport::accept_from(listener);
  {
    std::lock_guard<std::mutex> lock(server_listener_mutex_);
    if (server_listener_ == listener) {
      server_listener_ = kInvalidSocket;
    }
  }
  transport::close_socket(listener);
  if (accepted.socket_handle() == kInvalidSocket) {
    set_last_error(transport::last_tcp_error());
    return -3;
  }

  const std::string peer_ip = accepted.peer_address();
  auto session = std::make_unique<transport::TlsTransport>(
      std::move(accepted), transport::TlsMode::kServer, psk, psk_length);
  if (!session->handshake()) {
    set_last_error("TLS server handshake failed");
    return -4;
  }

  std::string request;
  if (!recv_protocol_message(*session, &request)) {
    set_last_error("Failed to receive connection request");
    return -5;
  }

  std::string device_name;
  if (!parse_connection_request(request, &device_name)) {
    set_last_error("Invalid connection request");
    return -6;
  }

  wait_for_approval(device_name, peer_ip);
  ApprovalDecision decision = ApprovalDecision::kRejected;
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    decision = approval_decision_;
  }
  if (decision == ApprovalDecision::kUnavailable) {
    send_protocol_message(*session, "UNAVAILABLE");
    set_last_error("Approval UI is not available on host");
    return -9;
  }
  if (decision != ApprovalDecision::kApproved) {
    send_protocol_message(*session, kRejected);
    set_last_error("Connection request rejected");
    return -7;
  }

  if (!send_protocol_message(*session, kApproved)) {
    set_last_error("Failed to send approval response");
    return -8;
  }

  server_session_ = std::move(session);
  return 0;
}

int32_t Runtime::start_server_async(uint16_t port, const uint8_t* psk,
                                    uint32_t psk_length) {
  if (server_running_.exchange(true)) {
    return 0;
  }
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
  std::vector<uint8_t> psk_copy(psk, psk + psk_length);
  server_thread_ = std::thread([this, port, psk_copy = std::move(psk_copy)] {
    while (server_running_.load()) {
      const int32_t result = start_server(port, psk_copy.data(),
                                          static_cast<uint32_t>(psk_copy.size()));
      if (result == 0 || result == -2) {
        break;
      }
    }
    server_running_.store(false);
  });
  return 0;
}

int32_t Runtime::set_approval_port(int64_t native_port) {
  if (!dart_api_available()) {
    (void)native_port;
    set_last_error("Dart NativePort API unavailable");
    return -2;
  }
  std::lock_guard<std::mutex> lock(state_mutex_);
  approval_port_ = native_port;
  return 0;
}

int32_t Runtime::approve_pending(int32_t request_id) {
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (pending_request_id_ != request_id || pending_request_id_ == 0) {
      last_error_ = "No matching pending approval request";
      return -1;
    }
    approval_decision_ = ApprovalDecision::kApproved;
  }
  approval_cv_.notify_all();
  return 0;
}

int32_t Runtime::reject_pending(int32_t request_id) {
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (pending_request_id_ != request_id || pending_request_id_ == 0) {
      last_error_ = "No matching pending approval request";
      return -1;
    }
    approval_decision_ = ApprovalDecision::kRejected;
  }
  approval_cv_.notify_all();
  return 0;
}

int32_t Runtime::connect_client(const char* ip_address, uint16_t port,
                                 const uint8_t* psk, uint32_t psk_length) {
  return connect_client(ip_address, port, psk, psk_length, "Tuba client");
}

int32_t Runtime::connect_client(const char* ip_address, uint16_t port,
                                const uint8_t* psk, uint32_t psk_length,
                                const char* device_name) {
  auto session = std::make_unique<transport::TlsTransport>(
      transport::TcpTransport(), transport::TlsMode::kClient, psk, psk_length);
  if (!session->connect(ip_address, port)) {
    set_last_error(transport::last_tcp_error()[0] != '\0' ? transport::last_tcp_error()
                                                          : "TLS client connect failed");
    return -2;
  }
  if (!send_protocol_message(*session, make_connection_request(device_name))) {
    set_last_error("Failed to send connection request");
    return -4;
  }
  std::string response;
  if (!recv_protocol_message(*session, &response)) {
    set_last_error("Failed to receive approval response");
    return -5;
  }
  if (response == kRejected) {
    set_last_error("Host rejected connection");
    return -3;
  }
  if (response == "UNAVAILABLE") {
    set_last_error("Host is reachable, but its approval UI is not available");
    return -7;
  }
  if (response != kApproved) {
    set_last_error("Invalid approval response");
    return -6;
  }
  client_session_ = std::move(session);
  return 0;
}

void Runtime::disconnect_client() {
  stop_frame_stream();
  if (client_session_) {
    client_session_->disconnect();
    client_session_.reset();
  }
}

int32_t Runtime::start_discovery(uint16_t discovery_port,
                                 const char* device_name, uint16_t tcp_port) {
  return discovery_responder_->start(discovery_port, device_name, tcp_port) ? 0
                                                                            : -1;
}

int32_t Runtime::query_discovery(int64_t native_port, uint16_t discovery_port,
                                 uint32_t timeout_ms) {
  if (!dart_api_available()) {
    (void)native_port;
    (void)discovery_port;
    (void)timeout_ms;
    set_last_error("Dart NativePort API unavailable");
    return -2;
  }
  const auto devices = transport::UdpDiscovery::query(discovery_port, timeout_ms);
  for (const auto& device : devices) {
    post_discovered_device_to_dart(native_port, device);
  }
  return static_cast<int32_t>(devices.size());
}

const char* Runtime::last_error() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return last_error_.c_str();
}

int32_t Runtime::start_capture() {
  if (!capture_) {
    capture_ = capture::create_platform_capture();
  }
  return capture_ && capture_->start() ? 0 : -1;
}

void Runtime::stop_capture() {
  stop_frame_stream();
  if (capture_) {
    capture_->stop();
  }
}

int32_t Runtime::start_frame_stream(int64_t native_port) {
  if (!dart_api_available()) {
    (void)native_port;
    return -2;
  }
  if (!capture_) {
    capture_ = capture::create_platform_capture();
  }
  if (!capture_ || !capture_->start()) {
    return -3;
  }
  stop_frame_stream();
  frame_stream_running_.store(true);
  frame_stream_thread_ =
      std::thread(&Runtime::frame_stream_loop, this, native_port);
  return 0;
}

void Runtime::stop_frame_stream() {
  frame_stream_running_.store(false);
  if (frame_stream_thread_.joinable()) {
    frame_stream_thread_.join();
  }
}

int32_t Runtime::send_mouse_move(double x, double y) {
  (void)x;
  (void)y;
  return client_session_ ? 0 : -1;
}

int32_t Runtime::send_mouse_down(double x, double y, int32_t button) {
  (void)x;
  (void)y;
  (void)button;
  return client_session_ ? 0 : -1;
}

int32_t Runtime::send_mouse_up(double x, double y, int32_t button) {
  (void)x;
  (void)y;
  (void)button;
  return client_session_ ? 0 : -1;
}

void Runtime::frame_stream_loop(int64_t native_port) {
  while (frame_stream_running_.load()) {
    if (capture_) {
      post_frame_to_dart(native_port, capture_->next_frame());
    }
    std::this_thread::sleep_for(kFrameInterval);
  }
}

int32_t Runtime::wait_for_approval(const std::string& device_name,
                                   const std::string& ip_address) {
  int32_t request_id = 0;
  int64_t port = 0;
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    request_id = next_request_id_++;
    pending_request_id_ = request_id;
    approval_decision_ = ApprovalDecision::kPending;
    port = approval_port_;
  }

  if (port == 0 || !post_approval_request_to_dart(port, request_id, device_name, ip_address)) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    pending_request_id_ = 0;
    approval_decision_ = ApprovalDecision::kUnavailable;
    last_error_ = "Approval NativePort is not available";
    return request_id;
  }

  std::unique_lock<std::mutex> lock(state_mutex_);
  approval_cv_.wait_for(lock, kApprovalTimeout, [this, request_id] {
    return pending_request_id_ == request_id &&
           approval_decision_ != ApprovalDecision::kPending;
  });
  if (approval_decision_ == ApprovalDecision::kPending) {
    approval_decision_ = ApprovalDecision::kRejected;
  }
  pending_request_id_ = 0;
  return request_id;
}

void Runtime::set_last_error(std::string message) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  last_error_ = std::move(message);
}

void Runtime::stop_server_thread() {
  server_running_.store(false);
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (pending_request_id_ != 0) {
      approval_decision_ = ApprovalDecision::kRejected;
    }
  }
  approval_cv_.notify_all();
  {
    std::lock_guard<std::mutex> lock(server_listener_mutex_);
    if (server_listener_ != kInvalidSocket) {
      transport::close_socket(server_listener_);
      server_listener_ = kInvalidSocket;
    }
  }
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
}

Runtime& runtime() {
  static Runtime instance;
  return instance;
}

}  // namespace rc
