#include <libhoard/pointer_policy.h>
#include <libhoard/cache.h>

#include <memory>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/weaken_policy.h>
#include <libhoard/max_size_policy.h>

SUITE(shared_pointer) {
  using cache_type = libhoard::cache<int, std::shared_ptr<int>,
        libhoard::pointer_policy<>,
        libhoard::weaken_policy,
        libhoard::max_size_policy>;

  void cause_expiry(cache_type& cache) {
    cache.emplace(2, std::make_shared<int>(2));
    cache.get(2);
    cache.get(2);
    cache.emplace(3, std::make_shared<int>(3));
    cache.get(3);
    cache.get(3);
  }

  TEST(shared_pointer_test) {
    auto cache = cache_type(libhoard::max_size_policy(2));
    auto test_value = std::make_shared<int>(1);
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
    CHECK_EQUAL(std::shared_ptr<int>(nullptr), cache.get(1).value_or(nullptr));
  }
}
