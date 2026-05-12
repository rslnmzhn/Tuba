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

extern "C" int32_t rc_initialize_dart_api(void* initialize_api_data) {
  if (initialize_api_data == nullptr) {
    return -1;
  }
  return rc::runtime().initialize_dart_api(initialize_api_data);
}

extern "C" int32_t rc_server_start(uint16_t port, const uint8_t* psk,
                                     uint32_t psk_length) {
  if (!valid_psk(psk, psk_length)) {
    return -1;
  }
  const uint16_t listen_port = port == 0U ? kDefaultTcpPort : port;
  return rc::runtime().start_server(listen_port, psk, psk_length);
}

extern "C" int32_t rc_server_start_async(uint16_t port, const uint8_t* psk,
                                           uint32_t psk_length) {
  if (!valid_psk(psk, psk_length)) {
    return -1;
  }
  const uint16_t listen_port = port == 0U ? kDefaultTcpPort : port;
  return rc::runtime().start_server_async(listen_port, psk, psk_length);
}

extern "C" int32_t rc_server_set_approval_port(int64_t native_port) {
  if (native_port == 0) {
    return -1;
  }
  return rc::runtime().set_approval_port(native_port);
}

extern "C" int32_t rc_server_approve_pending(int32_t request_id) {
  return rc::runtime().approve_pending(request_id);
}

extern "C" int32_t rc_server_reject_pending(int32_t request_id) {
  return rc::runtime().reject_pending(request_id);
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

extern "C" int32_t rc_client_connect_named(const char* ip_address, uint16_t port,
                                             const uint8_t* psk,
                                             uint32_t psk_length,
                                             const char* device_name) {
  if (ip_address == nullptr || device_name == nullptr || !valid_psk(psk, psk_length)) {
    return -1;
  }
  const uint16_t connect_port = port == 0U ? kDefaultTcpPort : port;
  return rc::runtime().connect_client(ip_address, connect_port, psk, psk_length,
                                      device_name);
}

extern "C" int32_t rc_client_disconnect(void) {
  rc::runtime().disconnect_client();
  return 0;
}

extern "C" int32_t rc_discovery_start(uint16_t port, const char* device_name) {
  const uint16_t discovery_port = port == 0U ? kDefaultDiscoveryPort : port;
  return rc::runtime().start_discovery(discovery_port, device_name,
                                       kDefaultTcpPort);
}

extern "C" int32_t rc_discovery_query(int64_t native_port, uint16_t port,
                                       uint32_t timeout_ms) {
  if (native_port == 0) {
    return -1;
  }
  const uint16_t discovery_port = port == 0U ? kDefaultDiscoveryPort : port;
  return rc::runtime().query_discovery(native_port, discovery_port, timeout_ms);
}

extern "C" const char* rc_last_error(void) {
  return rc::runtime().last_error();
}

extern "C" int32_t rc_capture_start(void) {
  return rc::runtime().start_capture();
}

extern "C" int32_t rc_capture_stop(void) {
  rc::runtime().stop_capture();
  return 0;
}

extern "C" int32_t rc_frame_stream_start(int64_t native_port) {
  if (native_port == 0) {
    return -1;
  }
  return rc::runtime().start_frame_stream(native_port);
}

extern "C" int32_t rc_frame_stream_stop(void) {
  rc::runtime().stop_frame_stream();
  return 0;
}

extern "C" int32_t rc_send_mouse_move(double x, double y) {
  return rc::runtime().send_mouse_move(x, y);
}

extern "C" int32_t rc_send_mouse_down(double x, double y, int32_t button) {
  return rc::runtime().send_mouse_down(x, y, button);
}

extern "C" int32_t rc_send_mouse_up(double x, double y, int32_t button) {
  return rc::runtime().send_mouse_up(x, y, button);
}
