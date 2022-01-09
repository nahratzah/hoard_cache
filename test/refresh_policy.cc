#include <libhoard/refresh_policy.h>

#include <chrono>
#include <chrono>
#include <system_error>
#include <tuple>
#include <list>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/resolver_policy.h>
#include <libhoard/detail/hashtable.h>

SUITE(refresh_policy) {
  class fixture {
    public:
    class resolver_impl {
      public:
      explicit resolver_impl(fixture* self)
      : self(self)
      {}

      auto operator()(int resolver_key) const -> std::tuple<std::string> {
        CHECK_EQUAL(3, resolver_key);

        auto result = self->resolver_values.front();
        if (self->resolver_values.size() > 1)
          self->resolver_values.pop_front();
        return result;
      }

      private:
      fixture* self;
    };

    using hashtable_type = libhoard::detail::hashtable<int, std::string, std::error_code,
          libhoard::refresh_policy<std::chrono::system_clock>,
          libhoard::resolver_policy<resolver_impl>>;

    void init(std::chrono::system_clock::duration refresh) {
      hashtable = std::make_unique<hashtable_type>(
          std::make_tuple(
              libhoard::resolver_policy<resolver_impl>(resolver_impl(this)),
              libhoard::refresh_policy<std::chrono::system_clock>(refresh)));
    }

    void init(std::chrono::system_clock::duration refresh, std::chrono::system_clock::duration idle_timer) {
      hashtable = std::make_unique<hashtable_type>(
          std::make_tuple(
              libhoard::resolver_policy<resolver_impl>(resolver_impl(this)),
              libhoard::refresh_policy<std::chrono::system_clock>(refresh, idle_timer)));
    }

    std::list<std::string> resolver_values;
    std::unique_ptr<hashtable_type> hashtable;
  };

  TEST_FIXTURE(fixture, refresh) {
    using namespace std::literals::chrono_literals;
    init(10s);
    resolver_values = { "first", "refresh_1", "refresh_2" };

    // Values are set to refresh after 10s.
    CHECK_EQUAL("first", std::get<1>(hashtable->get(3)));
    const auto base_timepoint = std::chrono::system_clock::now();

    // After 1s, the value is still the same.
    std::this_thread::sleep_until(base_timepoint + 1s);
    {
      const auto value = hashtable->get(3);
      if (std::chrono::system_clock::now() - base_timepoint < 10s)
        CHECK_EQUAL("first", std::get<1>(value));
    }

    // After 10s, the value gets refreshed.
    // Because the resolver is an immediate resolver, the refreshed value is immediately available.
    std::this_thread::sleep_until(base_timepoint + 11s);
    {
      REQUIRE CHECK(std::chrono::system_clock::now() - base_timepoint > 10s); // Need this for the test to work.
      const auto value = hashtable->get(3);
      REQUIRE CHECK(std::chrono::system_clock::now() - base_timepoint < 20s); // Need this for the test to work.
      CHECK_EQUAL("refresh_1", std::get<1>(value));
    }
  }

  TEST_FIXTURE(fixture, idle_timer) {
    using namespace std::literals::chrono_literals;
    init(1s, 5s);
    resolver_values = { "value" };

    CHECK_EQUAL("value", std::get<1>(hashtable->get(3)));
    const auto base_timepoint = std::chrono::system_clock::now();

    // After 1s, the value is still the same.
    std::this_thread::sleep_until(base_timepoint + 1s);
    {
      const auto value = hashtable->get_if_exists(3);
      if (std::chrono::system_clock::now() - base_timepoint < 5s)
        CHECK_EQUAL("value", std::get<1>(value));
    }

    // After idle_timer, the value should be gone, because it has not been queried.
    std::this_thread::sleep_until(base_timepoint + 6s);
    {
      const auto value = hashtable->get_if_exists(3);
      CHECK_EQUAL(0u, value.index());
    }
  }
}
