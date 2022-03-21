#include <libhoard/max_size_policy.h>

#include <string>
#include <system_error>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/hashtable.h>

SUITE(max_size_policy) {
  TEST(limiting_size) {
    constexpr unsigned int maxsize = 5;
    using libhoard::detail::hashtable;
    using libhoard::max_size_policy;
    using hashtable_type = hashtable<int, std::string, max_size_policy>;

    auto table = std::make_shared<hashtable_type>(max_size_policy(maxsize));
    for (unsigned int i = 0; i < maxsize; ++i)
      table->emplace(i, "bla");
    CHECK_EQUAL(maxsize, table->count()); // First `maxsize` elements don't cause anything to expire.

    for (unsigned int i = maxsize; i < maxsize * 2u; ++i) {
      table->emplace(i, "bla");
      CHECK_EQUAL(maxsize, table->count()); // Policy keeps cache size at desired `maxsize` elements.
    }
  }
}
