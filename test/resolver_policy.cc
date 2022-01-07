#include <libhoard/resolver_policy.h>

#include <string>
#include <system_error>
#include <tuple>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/hashtable.h>

SUITE(resolver_policy) {
  class fixture {
    public:
    struct resolver_impl {
      auto operator()(int n) const -> std::tuple<std::string::size_type, char> {
        return std::make_tuple(std::string::size_type(n), 'x');
      }
    };

    static_assert(libhoard::detail::has_resolver<libhoard::resolver_policy<resolver_impl>>::value);
    static_assert(libhoard::detail::hashtable_helper_<int, std::string, std::error_code, libhoard::resolver_policy<resolver_impl>>::has_resolver_policy,
        "expected the helper type to detect our resolver_policy");
    static_assert(!libhoard::detail::hashtable_helper_<int, std::string, std::error_code, libhoard::resolver_policy<resolver_impl>>::has_async_resolver_policy,
        "this is not an async resolver policy");

    using hashtable_type = libhoard::detail::hashtable<int, std::string, std::error_code, libhoard::resolver_policy<resolver_impl>>;

    std::unique_ptr<hashtable_type> hashtable = std::make_unique<hashtable_type>(std::make_tuple(libhoard::resolver_policy<resolver_impl>()));
  };

  TEST_FIXTURE(fixture, get) {
    auto three = hashtable->get(3);
    CHECK_EQUAL(1, three.index());
    CHECK_EQUAL(std::string("xxx"), std::get<1>(three));

    auto four = hashtable->get(4);
    CHECK_EQUAL(1, four.index());
    CHECK_EQUAL(std::string("xxxx"), std::get<1>(four));
  }
}
