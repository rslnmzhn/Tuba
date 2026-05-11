#ifndef RC_NATIVE_SOCKET_PLATFORM_H_
#define RC_NATIVE_SOCKET_PLATFORM_H_

#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using rc_socket_t = SOCKET;
constexpr rc_socket_t kInvalidSocket = INVALID_SOCKET;
#else
#include <netinet/in.h>
#include <sys/socket.h>
using rc_socket_t = int;
constexpr rc_socket_t kInvalidSocket = -1;
#endif

namespace rc::transport {

bool initialize_sockets();
void shutdown_sockets();
void close_socket(rc_socket_t socket_handle);
bool set_reuse_addr(rc_socket_t socket_handle);
bool set_broadcast(rc_socket_t socket_handle);
bool set_recv_timeout(rc_socket_t socket_handle, uint32_t timeout_ms);
bool socket_would_block();

}  // namespace rc::transport

#endif  // RC_NATIVE_SOCKET_PLATFORM_H_
