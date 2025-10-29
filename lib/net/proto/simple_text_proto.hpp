#ifndef __VJIK_NET_SIMPLE_TEXT_PROTO_HPP
#define __VJIK_NET_SIMPLE_TEXT_PROTO_HPP

#include "loop.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <print>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
inline int get_listener_socket() {
  int listener;
  int yes = 1;
  int rv;

  struct addrinfo hints, *ai, *p;
  std::memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(nullptr, "3000", &hints, &ai) != 0)) {
    std::cerr << "select server: " << gai_strerror(rv) << "\n";
    ::exit(1);
  }

  for (p = ai; p != nullptr; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0)  continue; 

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener);
      continue;
    }

    break;
  }

  freeaddrinfo(ai);

  if (p == nullptr) {
    return -1;
  }

  if (listen(listener, 10) == -1) {
    return -1;
  }

  return listener;
}

constexpr size_t packet_size = 512;
} // namespace

namespace vjik {
namespace net {
namespace proto {

class simple_text_proto {
public:
  simple_text_proto() : listener_(get_listener_socket()), loop_() {
    loop_.watch_read_ops_async(listener_, std::bind(accept_handler{this}));
    loop_.run();
  }

  ~simple_text_proto() { close(listener_); }

public:
  auto get_listener() const noexcept -> int { return listener_; }

private:
  class accept_handler {
  public:
    accept_handler(simple_text_proto *parent) : parent_(parent) {}

  public:
    bool operator()() {
      struct sockaddr_storage theiraddr{};
      socklen_t addrlen = sizeof(theiraddr);
      int clientfd =
          ::accept(parent_->listener_, reinterpret_cast<sockaddr *>(&theiraddr),
                   &addrlen);
      if (clientfd) {
        std::println("accept_handler::operator()");
        std::println("clientfd: {}", clientfd);
        parent_->loop_.watch_read_ops_async(
          clientfd,
          std::bind(read_handler{clientfd})
        );
      }
      return false;
    }

  private:
    simple_text_proto *parent_;
  };

class read_handler {
  public:
    read_handler(int fd) : fd_(fd) {
      std::println("read_handler::read_handler fd={}", fd_);
    }

    bool operator()() {
      std::println("read_handler::start_read_op fd={}", fd_);
      std::array<char, packet_size> buf{};
      ssize_t nbytes = ::read(fd_, buf.data(), buf.size());
      if (nbytes > 0) {
        buf[nbytes] = '\0';
        printf("%s", buf.data());
        return true;
      } else if (nbytes == 0) {
        std::println("disconnected fd={}", fd_);
        return true;
      } else {
        std::perror("read");
      }

      ::close(fd_);
      return true;
    }

  private:
    int fd_;
  };

private:
  int listener_;
  core::loop loop_;
};

} // namespace proto
} // namespace net
} // namespace vjik

#endif
