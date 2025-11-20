#ifndef __VJIK_NET_CORE_LOOP_HPP
#define __VJIK_NET_CORE_LOOP_HPP

#include <cstring>
#include <functional>
#include <netdb.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

#include "io_operations_queue.hpp"

namespace vjik {
namespace net {
namespace core {

class loop {
  using Handler = std::function<bool()>;

public:
  loop() 
    : shutdown_(false),
      read_queue_(),
      write_queue_() {};

  loop(loop &) = delete;
  loop &operator=(loop &) = delete;
  // TODO: move constructor/assigment....
  loop(loop &&);
  loop &operator=(loop &&);
  virtual ~loop() {}

public:
  auto run() {
    while (!shutdown_) {
      int max_fd = -1;

      fd_set read_fds;
      FD_ZERO(&read_fds);
      fd_set write_fds;
      FD_ZERO(&write_fds);

#if 0 
      for (const auto &it : read_queue_) {
        FD_SET(it.first, &read_fds);
        max_fd = std::max(max_fd, it.first);
      }

      timeval tv_buf = {1, 0};
      int nready = ::select(max_fd + 1, &read_fds, nullptr, nullptr, &tv_buf);

      if (nready > 0) {
        read_queue_.dispatch_io_operations();
        write_queue_.dispatch_io_operations();

        auto it = read_queue_.begin();
        while (it != read_queue_.end()) {
          auto fd = it->first;
          auto fn = it->second;
          ++it;
          if (FD_ISSET(fd, &read_fds)) {
            auto done = fn(); 
            if (done) { read_queue_.erase(fd); }
          }
        }
      }
#endif
    }
  }

  auto watch_read_ops_async(int sock_fd, Handler h) -> void {
    read_queue_.enqueue_op(sock_fd, h);
  }

  auto watch_write_ops_async(int sock_fd, Handler h) -> void {
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
