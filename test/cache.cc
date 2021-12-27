#include <libhoard/cache.h>

#include <string>

#include "UnitTest++/UnitTest++.h"

SUITE(cache) {
  class fixture {
    public:
    using cache_type = libhoard::cache<int, std::string>;

    fixture() {
      cache.emplace(1, "one");
      cache.emplace(2, "two");
    }

    cache_type cache;
  };

  TEST_FIXTURE(fixture, get_if_exists) {
    auto one = cache.get_if_exists(1);
    CHECK(one.has_value());
    if (one.has_value()) CHECK_EQUAL(std::string("one"), *one);

    auto two = cache.get_if_exists(2);
    CHECK(two.has_value());
    if (two.has_value()) CHECK_EQUAL(std::string("two"), *two);

    auto three = cache.get_if_exists(3);
    CHECK(!three.has_value());
  }

  TEST_FIXTURE(fixture, get) {
    auto one = cache.get(1);
    CHECK(one.has_value());
    if (one.has_value()) CHECK_EQUAL(std::string("one"), *one);

    auto two = cache.get(2);
    CHECK(two.has_value());
    if (two.has_value()) CHECK_EQUAL(std::string("two"), *two);

    auto three = cache.get(3);
    CHECK(!three.has_value());
  }

  TEST_FIXTURE(fixture, clear) {
    cache.clear();

    auto one = cache.get(1);
    CHECK(!one.has_value());

    auto two = cache.get(2);
    CHECK(!two.has_value());
  }

  TEST_FIXTURE(fixture, erase) {
    cache.erase(1);

    auto one = cache.get(1);
    CHECK(!one.has_value());

    auto two = cache.get(2);
    CHECK(two.has_value());
    if (two.has_value()) CHECK_EQUAL(std::string("two"), *two);
  }

  TEST_FIXTURE(fixture, emplace) {
    cache.emplace(17, "seventeen");

    auto seventeen = cache.get(17);
    CHECK(seventeen.has_value());
    CHECK_EQUAL(std::string("seventeen"), *seventeen);
  }

  TEST_FIXTURE(fixture, piecewise_emplace) {
    cache.emplace(std::piecewise_construct, std::forward_as_tuple(17), std::forward_as_tuple("seventeen"));

    auto seventeen = cache.get(17);
    CHECK(seventeen.has_value());
    CHECK_EQUAL(std::string("seventeen"), *seventeen);
  }

  TEST_FIXTURE(fixture, emplace_replaces) {
    cache.emplace(1, "bla bla chocoladevla");

    auto one = cache.get(1);
    CHECK(one.has_value());
    CHECK_EQUAL(std::string("bla bla chocoladevla"), *one);
  }

  TEST_FIXTURE(fixture, piecewise_emplace_replaces) {
    cache.emplace(std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple("bla bla chocoladevla"));

    auto one = cache.get(1);
    CHECK(one.has_value());
    CHECK_EQUAL(std::string("bla bla chocoladevla"), *one);
  }

  TEST_FIXTURE(fixture, get_or_emplace_on_existing_element) {
    auto result = cache.get_or_emplace(1, "bla bla bla chocoladevla");
    CHECK_EQUAL("one", result); // Returned existing value.

    auto one = cache.get(1);
    CHECK(one.has_value());
    CHECK_EQUAL(std::string("one"), *one); // No replacement happened.
  }

  TEST_FIXTURE(fixture, piecewise_get_or_emplace_on_existing_element) {
    auto result = cache.get_or_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::forward_as_tuple("bla bla bla chocoladevla"));
    CHECK_EQUAL("one", result); // Returned existing value.

    auto one = cache.get(1);
    CHECK(one.has_value());
    CHECK_EQUAL(std::string("one"), *one); // No replacement happened.
  }

  TEST_FIXTURE(fixture, get_or_emplace_on_miss) {
    auto result = cache.get_or_emplace(17, "bla bla bla chocoladevla");
    CHECK_EQUAL("bla bla bla chocoladevla", result); // Returned existing value.

    auto seventeen = cache.get(17);
    CHECK(seventeen.has_value());
    CHECK_EQUAL(std::string("bla bla bla chocoladevla"), *seventeen); // No replacement happened.
  }

  TEST_FIXTURE(fixture, piecewise_get_or_emplace_on_miss) {
    auto result = cache.get_or_emplace(std::piecewise_construct, std::forward_as_tuple(17), std::forward_as_tuple("bla bla bla chocoladevla"));
    CHECK_EQUAL("bla bla bla chocoladevla", result); // Returned existing value.

    auto seventeen = cache.get(17);
    CHECK(seventeen.has_value());
    CHECK_EQUAL(std::string("bla bla bla chocoladevla"), *seventeen); // No replacement happened.
  }
}
