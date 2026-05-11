#include "udp_discovery.h"

#include "socket_platform.h"

#include <cstdio>
#include <cstring>

#if !defined(_WIN32)
#include <arpa/inet.h>
#endif

namespace rc::transport {
namespace {

constexpr char kDiscoveryProbe[] = "TUBA_DISCOVER_V1";

}  // namespace

bool UdpDiscovery::announce_once(uint16_t port, const char* device_name,
                                 uint16_t tcp_port) {
  if (device_name == nullptr || !initialize_sockets()) {
    return false;
  }
  rc_socket_t socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_handle == kInvalidSocket) {
    return false;
  }

  set_broadcast(socket_handle);

  char payload[256]{};
  std::snprintf(payload, sizeof(payload), "TUBA_DEVICE_V1 name=%s tcp=%u",
                device_name, static_cast<unsigned>(tcp_port));

  sockaddr_in target{};
  target.sin_family = AF_INET;
  target.sin_port = htons(port);
  target.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  const int sent = sendto(socket_handle, payload, static_cast<int>(std::strlen(payload)),
                          0, reinterpret_cast<sockaddr*>(&target), sizeof(target));
  close_socket(socket_handle);
  return sent > 0;
}

bool UdpDiscovery::query_once(uint16_t port, uint32_t timeout_ms) {
  if (!initialize_sockets()) {
    return false;
  }
  rc_socket_t socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_handle == kInvalidSocket) {
    return false;
  }

  set_broadcast(socket_handle);
  set_recv_timeout(socket_handle, timeout_ms);

  sockaddr_in target{};
  target.sin_family = AF_INET;
  target.sin_port = htons(port);
  target.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  const int sent = sendto(socket_handle, kDiscoveryProbe,
                          static_cast<int>(std::strlen(kDiscoveryProbe)), 0,
                          reinterpret_cast<sockaddr*>(&target), sizeof(target));
  if (sent <= 0) {
    close_socket(socket_handle);
    return false;
  }

  char buffer[256]{};
  sockaddr_storage source{};
  socklen_t source_length = sizeof(source);
  const int received = recvfrom(socket_handle, buffer, sizeof(buffer) - 1U, 0,
                                reinterpret_cast<sockaddr*>(&source),
                                &source_length);
  close_socket(socket_handle);
  return received > 0;
}

}  // namespace rc::transport
