#ifndef RC_API_H_
#define RC_API_H_

#include <stdint.h>

#if defined(_WIN32)
#if defined(RC_NATIVE_BUILDING_LIBRARY)
#define RC_API __declspec(dllexport)
#else
#define RC_API __declspec(dllimport)
#endif
#else
#define RC_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

RC_API int32_t rc_native_abi_version(void);
RC_API int32_t rc_initialize_dart_api(void* initialize_api_data);
RC_API int32_t rc_server_start(uint16_t port, const uint8_t* psk,
                               uint32_t psk_length);
RC_API int32_t rc_server_start_async(uint16_t port, const uint8_t* psk,
                                     uint32_t psk_length);
RC_API int32_t rc_server_set_approval_port(int64_t native_port);
RC_API int32_t rc_server_approve_pending(int32_t request_id);
RC_API int32_t rc_server_reject_pending(int32_t request_id);
RC_API int32_t rc_client_connect(const char* ip_address, uint16_t port,
                                  const uint8_t* psk, uint32_t psk_length);
RC_API int32_t rc_client_connect_named(const char* ip_address, uint16_t port,
                                       const uint8_t* psk,
                                       uint32_t psk_length,
                                       const char* device_name);
RC_API int32_t rc_client_disconnect(void);
RC_API int32_t rc_discovery_start(uint16_t port, const char* device_name);
RC_API int32_t rc_discovery_query(int64_t native_port, uint16_t port,
                                  uint32_t timeout_ms);
RC_API const char* rc_last_error(void);
RC_API int32_t rc_capture_start(void);
RC_API int32_t rc_capture_stop(void);
RC_API int32_t rc_frame_stream_start(int64_t native_port);
RC_API int32_t rc_frame_stream_stop(void);
RC_API int32_t rc_send_mouse_move(double x, double y);
RC_API int32_t rc_send_mouse_down(double x, double y, int32_t button);
RC_API int32_t rc_send_mouse_up(double x, double y, int32_t button);

#ifdef __cplusplus
}
#endif

#endif  // RC_API_H_
