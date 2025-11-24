#include <gtest/gtest.h>
#include <unistd.h>
#include <fcntl.h>

#include "loop.hpp"

namespace vjik {
namespace net {
namespace core {

class LoopTest : public ::testing::Test {
protected:
  LoopTest() = default;
  ~LoopTest() override = default;

  void SetUp() override {
    assert(::pipe(pipe_fds_) == 0);
    read_fd_ = pipe_fds_[0];
    write_fd_ = pipe_fds_[1];
  }

  void TearDown() override {
    for (auto const &fd : { read_fd_, write_fd_ }) {
      ::close(fd);
    }
  }

  int read_fd_ = -1;
  int write_fd_ = -1;
  int pipe_fds_[2] = {-1, -1};
};

TEST_F(LoopTest, ShouldWatchForRead) {
  auto loop = core::event_loop{};
  bool hasBeenCalled = false;
  auto handler = [&]() { 
    hasBeenCalled = true;
    loop.shutdown();
    return true;
  };

  loop.watch_read_ops_async(read_fd_, handler);
  ::write(write_fd_, "42", 2);

  loop.run();

  EXPECT_TRUE(hasBeenCalled);
}

TEST_F(LoopTest, ShouldBeenCalledReadHanderFewTimes) {
  auto loop = core::event_loop{};
  int couter = 0;
  auto handler = [&]() { 
    ++couter; 
    if (couter == 5) {
      loop.shutdown();
    }
    return false;
  };

  EXPECT_EQ(couter, 0);

  loop.watch_read_ops_async(read_fd_, handler);
  ::write(write_fd_, "42", 2);
  ::write(write_fd_, "42", 2);
  ::write(write_fd_, "42", 2);
  ::write(write_fd_, "42", 2);
  ::write(write_fd_, "42", 2);

  loop.run();

  EXPECT_EQ(couter, 5);
}

} // namespace core
} // namespace net
} // namespace vjik
