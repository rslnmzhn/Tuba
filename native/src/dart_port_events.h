#ifndef RC_NATIVE_DART_PORT_EVENTS_H_
#define RC_NATIVE_DART_PORT_EVENTS_H_

#include "udp_discovery.h"

#include <cstdint>
#include <string>

namespace rc {

bool dart_api_available();
int32_t initialize_dart_api_dl(void* initialize_api_data);
bool post_approval_request_to_dart(int64_t native_port, int32_t request_id,
                                   const std::string& device_name,
                                   const std::string& ip_address);
bool post_discovered_device_to_dart(int64_t native_port,
                                    const transport::DiscoveredDevice& device);

}  // namespace rc

#endif  // RC_NATIVE_DART_PORT_EVENTS_H_
