#include <libhoard/max_age_policy.h>

#include <chrono>
#include <ostream>
#include <string>
#include <system_error>
#include <tuple>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/hashtable.h>

SUITE(max_age_policy) {
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

  TEST(max_age_test) {
    using hashtable_type = libhoard::detail::hashtable<int, std::string, std::error_code, libhoard::max_age_policy<test_clock>>;
    using namespace std::literals::chrono_literals;

    // Values are to expire after 10s.
    auto table = std::make_unique<hashtable_type>(std::make_tuple(libhoard::max_age_policy<test_clock>(10s)));
    table->emplace(3, "three");

    CHECK_EQUAL("three", std::get<1>(table->get(3)));

    // After 1s, the value is still valid.
    test_clock::set_time(1s);
    CHECK_EQUAL("three", std::get<1>(table->get(3)));

    // After 9.999s, the value is still valid.
    test_clock::set_time(9999ms);
    CHECK_EQUAL("three", std::get<1>(table->get(3)));

    // After 10s, the value is no longer valid.
    test_clock::set_time(10s);
    CHECK_EQUAL(/* expired */ 0u, table->get(3).index());

    // After 11s, the value is (still) no longer valid.
    test_clock::set_time(11s);
    CHECK_EQUAL(/* expired */ 0u, table->get(3).index());
  }
}
