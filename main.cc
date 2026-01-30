#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
namespace {
constexpr int kBacklog = 128;
constexpr size_t kBufferSize = 16 * 1024;
int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void close_quietly(int fd) {
  if (fd >= 0)
    ::close(fd);
}

std::string sockaddr_to_string(const sockaddr_storage& ss) {
  char host[INET6_ADDRSTRLEN] = {};
  uint16_t port = 0;

  if (ss.ss_family == AF_INET) {
    const auto* addr = reinterpret_cast<const sockaddr_in*>(&ss);
    if (!inet_ntop(AF_INET, &addr->sin_addr, host, sizeof(host))) {
      return "<invalid>";
    }
    port = ntohs(addr->sin_port);
  } else if (ss.ss_family == AF_INET6) {
    const auto* addr = reinterpret_cast<const sockaddr_in6*>(&ss);
    if (!inet_ntop(AF_INET6, &addr->sin6_addr, host, sizeof(host))) {
      return "<invalid>";
    }
    port = ntohs(addr->sin6_port);
  } else {
    return "<unknown>";
  }

  return std::string(host) + ":" + std::to_string(port);
}

bool send_all(int fd, const char* data, size_t len) {
  size_t sent = 0;
  while (sent < len) {
#ifdef MSG_NOSIGNAL
    const int flags = MSG_NOSIGNAL;
#else
    const int flags = 0;
#endif
    ssize_t n = ::send(fd, data + sent, len - sent, flags);
    if (n > 0) {
      sent += static_cast<size_t>(n);
      continue;
    }
    if (n == -1 && (errno == EINTR))
      continue;
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
      return false;
    return false;
  }
  return true;
}

int create_listen_socket(uint16_t port) {
  int listen_fd = ::socket(AF_INET6, SOCK_STREAM, 0);
  if (listen_fd == -1)
    return -1;

  int one = 1;
  (void) ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#ifdef SO_REUSEPORT
  (void) ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif

  int v6only = 0;
  (void) ::setsockopt(listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only,
                      sizeof(v6only));

  sockaddr_in6 addr = {};
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = in6addr_any;
  addr.sin6_port = htons(port);

  if (::bind(listen_fd, reinterpret_cast<const sockaddr*>(&addr),
             sizeof(addr)) == -1) {
    close_quietly(listen_fd);
    return -1;
  }
  if (::listen(listen_fd, kBacklog) == -1) {
    close_quietly(listen_fd);
    return -1;
  }
  if (set_nonblocking(listen_fd) == -1) {
    close_quietly(listen_fd);
    return -1;
  }
  return listen_fd;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef SIGPIPE
  std::signal(SIGPIPE, SIG_IGN);
#endif

  uint16_t port = 12345;
  if (argc >= 2) {
    try {
      int p = std::stoi(argv[1]);
      if (p <= 0 || p > 65535)
        throw std::out_of_range("port");
      port = static_cast<uint16_t>(p);
    } catch (...) {
      std::cerr << "Usage: " << argv[0] << " [port]\n";
      return 2;
    }
  }

  int listen_fd = create_listen_socket(port);
  if (listen_fd == -1) {
    std::cerr << "Failed to listen on port " << port << ": "
              << std::strerror(errno) << "\n";
    return 1;
  }

  std::cout << "Echo server listening on 0.0.0.0:" << port
            << " and [::]:" << port << "\n";

  std::vector<pollfd> fds;
  fds.push_back(pollfd{listen_fd, POLLIN, 0});

  std::vector<char> buffer(kBufferSize);

  while (true) {
    int rc = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), -1);
    if (rc == -1) {
      if (errno == EINTR)
        continue;
      std::cerr << "poll() failed: " << std::strerror(errno) << "\n";
      break;
    }

    for (size_t i = 0; i < fds.size(); ++i) {
      if (fds[i].revents == 0)
        continue;

      if (fds[i].fd == listen_fd) {
        if (fds[i].revents & POLLIN) {
          while (true) {
            sockaddr_storage ss = {};
            socklen_t slen = sizeof(ss);
            int client_fd =
                ::accept(listen_fd, reinterpret_cast<sockaddr*>(&ss), &slen);
            if (client_fd == -1) {
              if (errno == EINTR)
                continue;
              if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
              std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
              break;
            }

            (void) set_nonblocking(client_fd);
#ifdef SO_NOSIGPIPE
            int one = 1;
            (void) ::setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &one,
                                sizeof(one));
#endif

            std::cout << "Client connected: " << sockaddr_to_string(ss) << "\n";
            fds.push_back(pollfd{client_fd, POLLIN, 0});
          }
        }
        continue;
      }

      const int client_fd = fds[i].fd;
      if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
        close_quietly(client_fd);
        fds.erase(fds.begin() + static_cast<long>(i));
        --i;
        continue;
      }

      if (fds[i].revents & POLLIN) {
        while (true) {
          ssize_t n = ::recv(client_fd, buffer.data(), buffer.size(), 0);
          if (n > 0) {
            if (!send_all(client_fd, buffer.data(), static_cast<size_t>(n))) {
              // If socket would block, keep it simple and drop the connection.
              close_quietly(client_fd);
              fds.erase(fds.begin() + static_cast<long>(i));
              --i;
              break;
            }
            continue;
          }
          if (n == 0) {
            close_quietly(client_fd);
            fds.erase(fds.begin() + static_cast<long>(i));
            --i;
            break;
          }
          if (errno == EINTR)
            continue;
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
          close_quietly(client_fd);
          fds.erase(fds.begin() + static_cast<long>(i));
          --i;
          break;
        }
      }
    }
  }

  for (const auto& pfd : fds)
    close_quietly(pfd.fd);
  return 0;
}
