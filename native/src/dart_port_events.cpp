#include "dart_port_events.h"

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

bool dart_api_available() {
#if RC_HAS_DART_API_DL
  return true;
#else
  return false;
#endif
}

int32_t initialize_dart_api_dl(void* initialize_api_data) {
#if RC_HAS_DART_API_DL
  return Dart_InitializeApiDL(initialize_api_data) == 0 ? 0 : -1;
#else
  (void)initialize_api_data;
  return -2;
#endif
}

bool post_approval_request_to_dart(int64_t native_port, int32_t request_id,
                                   const std::string& device_name,
                                   const std::string& ip_address) {
#if RC_HAS_DART_API_DL
  Dart_CObject type;
  type.type = Dart_CObject_kString;
  type.value.as_string = const_cast<char*>("approval_request");
  Dart_CObject id;
  id.type = Dart_CObject_kInt32;
  id.value.as_int32 = request_id;
  Dart_CObject name;
  name.type = Dart_CObject_kString;
  name.value.as_string = const_cast<char*>(device_name.c_str());
  Dart_CObject ip;
  ip.type = Dart_CObject_kString;
  ip.value.as_string = const_cast<char*>(ip_address.c_str());
  Dart_CObject* values[] = {&type, &id, &name, &ip};
  Dart_CObject message;
  message.type = Dart_CObject_kArray;
  message.value.as_array.length = 4;
  message.value.as_array.values = values;
  return Dart_PostCObject_DL(native_port, &message);
#else
  (void)native_port;
  (void)request_id;
  (void)device_name;
  (void)ip_address;
  return false;
#endif
}

bool post_discovered_device_to_dart(int64_t native_port,
                                    const transport::DiscoveredDevice& device) {
#if RC_HAS_DART_API_DL
  Dart_CObject type;
  type.type = Dart_CObject_kString;
  type.value.as_string = const_cast<char*>("device");
  Dart_CObject name;
  name.type = Dart_CObject_kString;
  name.value.as_string = const_cast<char*>(device.name.c_str());
  Dart_CObject ip;
  ip.type = Dart_CObject_kString;
  ip.value.as_string = const_cast<char*>(device.ip_address.c_str());
  Dart_CObject port;
  port.type = Dart_CObject_kInt32;
  port.value.as_int32 = static_cast<int32_t>(device.tcp_port);
  Dart_CObject* values[] = {&type, &name, &ip, &port};
  Dart_CObject message;
  message.type = Dart_CObject_kArray;
  message.value.as_array.length = 4;
  message.value.as_array.values = values;
  return Dart_PostCObject_DL(native_port, &message);
#else
  (void)native_port;
  (void)device;
  return false;
#endif
}

}  // namespace rc
