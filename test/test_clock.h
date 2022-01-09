#pragma once

#include <chrono>

class test_clock
: public std::chrono::system_clock
{
  public:
  static auto now() noexcept -> std::chrono::time_point<std::chrono::system_clock> {
    return now_value;
  }

  static auto set_time(std::chrono::milliseconds delta_from_base) -> void {
    now_value = base + delta_from_base;
  }

  static inline constexpr bool is_steady = true;

  private:
  static inline const std::chrono::time_point<std::chrono::system_clock> base = std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(0));
  static inline std::chrono::time_point<std::chrono::system_clock> now_value = base;
};
