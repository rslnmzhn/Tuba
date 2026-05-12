#ifndef RC_NATIVE_APPROVAL_PROTOCOL_H_
#define RC_NATIVE_APPROVAL_PROTOCOL_H_

#include "tls_transport.h"

#include <string>

namespace rc {

constexpr int32_t kClientRejected = -3;

std::string make_connection_request(const char* device_name);
bool parse_connection_request(const std::string& message,
                              std::string* device_name);
bool send_protocol_message(transport::TlsTransport& session,
                           const std::string& message);
bool recv_protocol_message(transport::TlsTransport& session,
                           std::string* message);

}  // namespace rc

#endif  // RC_NATIVE_APPROVAL_PROTOCOL_H_
