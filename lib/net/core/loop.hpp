#ifndef __VJIK_NET_CORE_LOOP_HPP
#define __VJIK_NET_CORE_LOOP_HPP

#include <cstring>
#include <functional>
#include <netdb.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

#include "io_operations_queue.hpp"

namespace {

constexpr int invalid_socket = -1;

class fd_set_adapter
{
public:
  fd_set_adapter()
    : max_descriptor_(invalid_socket)
  {
    FD_ZERO(&fd_set_);
  }

  void set(int descriptor)
  {
    if (max_descriptor_ == invalid_socket || descriptor > max_descriptor_)
      max_descriptor_ = descriptor;
    FD_SET(descriptor, &fd_set_);
  }

  bool is_set(int descriptor) const
  {
    return FD_ISSET(descriptor, &fd_set_) != 0;
  }

  operator fd_set*()
  {
    return &fd_set_;
  }

  int max_descriptor() const
  {
    return max_descriptor_;
  }

private:
  fd_set fd_set_;
  int max_descriptor_;
};

}

namespace vjik {
namespace net {
namespace core {

class event_loop {
  using Handler = std::function<bool()>;

public:
  event_loop() 
    : shutdown_(false),
      read_queue_(),
      write_queue_() {};

  event_loop(event_loop &) = delete;
  event_loop &operator=(event_loop &) = delete;
  // TODO: move constructor/assigment....
  event_loop(event_loop &&);
  event_loop &operator=(event_loop &&);
  virtual ~event_loop() {}

public:
  auto run() {
    while (!shutdown_) {
      int max_fd = -1;

      fd_set_adapter read_fds;
      read_queue_.get_descriptors(read_fds);
      fd_set_adapter write_fds;
      write_queue_.get_descriptors(write_fds);
      max_fd = std::max(read_fds.max_descriptor(), write_fds.max_descriptor());

      timeval tv_buf = {1, 0};
      int nready = ::select(max_fd + 1, read_fds, write_fds, nullptr, &tv_buf);

      if (nready > 0) {
        read_queue_.dispatch_io_operations(read_fds);
        write_queue_.dispatch_io_operations(write_fds);
      }

      read_queue_.cleanup_operations();
      write_queue_.cleanup_operations();
    }
  }

  auto shutdown() noexcept {
    shutdown_ -= true;
  }

  auto watch_read_ops_async(int sock_fd, Handler h) {
    read_queue_.enqueue_op(sock_fd, h);
  }

  auto watch_write_ops_async(int sock_fd, Handler h) {
    write_queue_.enqueue_op(sock_fd, h);
  }

private:
  bool shutdown_;
  io_operations_queue<int> read_queue_;
  io_operations_queue<int> write_queue_;
};

} // namespace core
} // namespace net
} // namespace vjik
#endif
