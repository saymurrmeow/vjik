#include "stun.hpp"

namespace stun = vjik::net::stun;

int main() {
  try {
    auto handler = stun::proto{};
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}
