#include "rc_runtime.h"

#include "capture_factory.h"
#include "socket_platform.h"
#include "tcp_transport.h"
#include "tls_transport.h"
#include "udp_discovery_responder.h"

#include <chrono>
#include <cstring>
#include <utility>
#include <vector>

#if __has_include(<dart_api_dl.h>)
#include <dart_api_dl.h>
#define RC_HAS_DART_API_DL 1
#else
#define RC_HAS_DART_API_DL 0
#endif

namespace rc {
namespace {

constexpr int kListenBacklog = 1;
constexpr auto kFrameInterval = std::chrono::milliseconds(33);

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

Runtime::~Runtime() = default;

int32_t Runtime::initialize_dart_api(void* initialize_api_data) {
#if RC_HAS_DART_API_DL
  return Dart_InitializeApiDL(initialize_api_data) == 0 ? 0 : -1;
#else
  (void)initialize_api_data;
  return -2;
#endif
}

int32_t Runtime::start_server(uint16_t port, const uint8_t* psk,
                              uint32_t psk_length) {
  const rc_socket_t listener =
      transport::TcpTransport::listen_on(port, kListenBacklog);
  if (listener == kInvalidSocket) {
    return -2;
  }

  transport::TcpTransport accepted =
      transport::TcpTransport::accept_from(listener);
  transport::close_socket(listener);
  if (accepted.socket_handle() == kInvalidSocket) {
    return -3;
  }

  auto session = std::make_unique<transport::TlsTransport>(
      std::move(accepted), transport::TlsMode::kServer, psk, psk_length);
  if (!session->handshake()) {
    return -4;
  }
  server_session_ = std::move(session);
  return 0;
}

int32_t Runtime::connect_client(const char* ip_address, uint16_t port,
                                 const uint8_t* psk, uint32_t psk_length) {
  auto session = std::make_unique<transport::TlsTransport>(
      transport::TcpTransport(), transport::TlsMode::kClient, psk, psk_length);
  if (!session->connect(ip_address, port)) {
    return -2;
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
#if !RC_HAS_DART_API_DL
  (void)native_port;
  return -2;
#else
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
#endif
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

Runtime& runtime() {
  static Runtime instance;
  return instance;
}

}  // namespace rc
