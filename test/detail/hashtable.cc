#include <libhoard/detail/hashtable.h>

#include <functional>
#include <string>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/mapped_type.h>

class hashtable_fixture {
  public:
  using hashtable_type = libhoard::detail::hashtable<std::string, std::string>;

  template<typename... Args>
  auto init_test(Args&&... args) -> void {
    hashtable = std::make_shared<hashtable_type>(std::forward<Args>(args)...);
  }

  std::shared_ptr<hashtable_type> hashtable;
};


SUITE(hashtable) {
  TEST_FIXTURE(hashtable_fixture, constructor) {
    init_test();

    CHECK(hashtable->empty());
    CHECK_EQUAL(0u, hashtable->size());
  }

  TEST_FIXTURE(hashtable_fixture, construct_with_max_load_factor) {
    init_test(17.0f);
    CHECK_CLOSE(17.0, hashtable->max_load_factor(), 0.000001);
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value) {
    init_test();

    hashtable->emplace(std::string("key"), std::string("value"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(1u, hashtable->size());
    CHECK_EQUAL(
        std::string("value"),
        std::get<1>(hashtable->get_if_exists("key")));
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value_using_tuples) {
    init_test();

    hashtable->emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::string("key")), std::forward_as_tuple("value"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(1u, hashtable->size());
    CHECK_EQUAL(
        std::string("value"),
        std::get<1>(hashtable->get_if_exists("key")));
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value_expires_others_with_same_key) {
    init_test();

    hashtable->emplace(std::string("key"), std::string("value 1"));
    hashtable->emplace(std::string("key"), std::string("value 2"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(
        std::string("value 2"),
        std::get<1>(hashtable->get_if_exists("key")));
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value_using_tuples_expires_others_with_same_key) {
    init_test();

    hashtable->emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::string("key")), std::forward_as_tuple("value 1"));
    hashtable->emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::string("key")), std::forward_as_tuple("value 2"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(
        std::string("value 2"),
        std::get<1>(hashtable->get_if_exists("key")));
  }

  TEST_FIXTURE(hashtable_fixture, get_if_present_on_empty_table) {
    init_test();

    auto get_result = hashtable->get_if_exists("key");
    CHECK_EQUAL(0u, get_result.index());
  }

  TEST_FIXTURE(hashtable_fixture, get_if_present_finds_a_value) {
    init_test();
    hashtable->emplace(std::string("key_1"), std::string("value_1"));
    hashtable->emplace(std::string("key_2"), std::string("value_2"));
    hashtable->emplace(std::string("key_3"), std::string("value_3"));

    auto get_result_1 = hashtable->get_if_exists("key_1");
    CHECK_EQUAL(std::string("value_1"), std::get<1>(get_result_1));

    auto get_result_2 = hashtable->get_if_exists("key_2");
    CHECK_EQUAL(std::string("value_2"), std::get<1>(get_result_2));

    auto get_result_3 = hashtable->get_if_exists("key_3");
    CHECK_EQUAL(std::string("value_3"), std::get<1>(get_result_3));
  }

  TEST_FIXTURE(hashtable_fixture, expire) {
    init_test();
    hashtable->emplace(std::string("key_1"), std::string("value_1"));
    hashtable->emplace(std::string("key_2"), std::string("value_2"));
    hashtable->emplace(std::string("key_3"), std::string("value_3"));

    hashtable->expire(std::string("key_3"));

    auto get_result = hashtable->get_if_exists("key_3");
    CHECK(get_result.index() == 0);

    // Other keys are not expired.
    auto get_result_1 = hashtable->get_if_exists("key_1");
    CHECK_EQUAL(std::string("value_1"), std::get<1>(get_result_1));

    auto get_result_2 = hashtable->get_if_exists("key_2");
    CHECK_EQUAL(std::string("value_2"), std::get<1>(get_result_2));
  }
}
