#include <libhoard/pointer_policy.h>
#include <libhoard/cache.h>

#include <memory>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/weaken_policy.h>
#include <libhoard/max_size_policy.h>

#include <cycle_ptr/cycle_ptr.h>

SUITE(cycle_pointer) {
  class fixture {
    public:
    struct mock_ownership
    : cycle_ptr::cycle_base
    {};

    struct member_constructor_args {
      explicit member_constructor_args(mock_ownership* mo)
      : mo(mo)
      {}

      auto operator()(const cycle_ptr::cycle_gptr<int>& p) const -> std::tuple<mock_ownership&, const cycle_ptr::cycle_gptr<int>&> {
        return std::tuple<mock_ownership&, const cycle_ptr::cycle_gptr<int>&>(*mo, p);
      }

      mock_ownership* mo;
    };

    using cache_type = libhoard::cache<int, cycle_ptr::cycle_gptr<int>,
          libhoard::pointer_policy<cycle_ptr::cycle_weak_ptr<int>, cycle_ptr::cycle_member_ptr<int>, member_constructor_args>,
          libhoard::weaken_policy,
          libhoard::max_size_policy>;

    void cause_expiry(cache_type& cache) {
      cache.emplace(2, cycle_ptr::make_cycle<int>(2));
      cache.get(2);
      cache.get(2);
      cache.emplace(3, cycle_ptr::make_cycle<int>(3));
      cache.get(3);
      cache.get(3);
    }

    cycle_ptr::cycle_gptr<mock_ownership> mock = cycle_ptr::make_cycle<mock_ownership>();
    cache_type cache = cache_type(
        libhoard::pointer_policy<cycle_ptr::cycle_weak_ptr<int>, cycle_ptr::cycle_member_ptr<int>, member_constructor_args>(member_constructor_args(mock.get())),
        libhoard::max_size_policy(2),
        libhoard::pointer_policy<>());
  };

  TEST_FIXTURE(fixture, cycle_pointer_test) {
    auto test_value = cycle_ptr::make_cycle<int>(1);
    cache.emplace(1, test_value);
    REQUIRE CHECK_EQUAL(test_value, cache.get(1).value_or(nullptr));

    // Despite being over size, and the other elements being more "hot",
    // the weaken-policy should keep it in the cache.
    cause_expiry(cache);
    CHECK_EQUAL(test_value, cache.get(1).value_or(nullptr));

    // Once the pointer has no more "strong" references, it should expire
    // when the cache is pushed to expire the value.
    test_value.reset();
    cause_expiry(cache);
    CHECK_EQUAL(cycle_ptr::cycle_gptr<int>(nullptr), cache.get(1).value_or(nullptr));
  }
}
