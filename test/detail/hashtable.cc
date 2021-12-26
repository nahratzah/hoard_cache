#include <libhoard/detail/hashtable.h>

#include <functional>
#include <string>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/mapped_type.h>

class hashtable_fixture {
  public:
  using hashtable_type = libhoard::detail::hashtable<std::string, libhoard::detail::mapped_value<std::string, std::allocator<std::string>, std::error_code>>;

  template<typename... Args>
  auto init_test(Args&&... args) -> void {
    hashtable = std::make_shared<hashtable_type>(std::forward<Args>(args)...);
    hashtable->init();
  }

  std::shared_ptr<hashtable_type> hashtable;
};


SUITE(hashtable) {
  TEST_FIXTURE(hashtable_fixture, constructor) {
    init_test(std::tuple<>());

    CHECK(hashtable->empty());
    CHECK_EQUAL(0u, hashtable->size());
  }

  TEST_FIXTURE(hashtable_fixture, construct_with_max_load_factor) {
    init_test(std::tuple<>(), 17.0f);
    CHECK_CLOSE(17.0, hashtable->max_load_factor(), 0.000001);
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value) {
    init_test(std::tuple<>());

    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key"), std::string("value"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(1u, hashtable->size());
    CHECK_EQUAL(
        std::string("value"),
        std::get<1>(
            hashtable->get_if_exists(
                std::hash<std::string>()("key"),
                std::bind(std::equal_to<std::string>(), "key", std::placeholders::_1))));
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value_using_tuples) {
    init_test(std::tuple<>());

    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::piecewise_construct,
        std::forward_as_tuple(std::string("key")), std::forward_as_tuple("value"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(1u, hashtable->size());
    CHECK_EQUAL(
        std::string("value"),
        std::get<1>(
            hashtable->get_if_exists(
                std::hash<std::string>()("key"),
                std::bind(std::equal_to<std::string>(), "key", std::placeholders::_1))));
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value_expires_others_with_same_key) {
    init_test(std::tuple<>());

    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key"), std::string("value 1"));
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key"), std::string("value 2"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(
        std::string("value 2"),
        std::get<1>(
            hashtable->get_if_exists(
                std::hash<std::string>()("key"),
                std::bind(std::equal_to<std::string>(), "key", std::placeholders::_1))));
  }

  TEST_FIXTURE(hashtable_fixture, emplace_value_using_tuples_expires_others_with_same_key) {
    init_test(std::tuple<>());

    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::piecewise_construct,
        std::forward_as_tuple(std::string("key")), std::forward_as_tuple("value 1"));
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::piecewise_construct,
        std::forward_as_tuple(std::string("key")), std::forward_as_tuple("value 2"));

    CHECK(!hashtable->empty());
    CHECK_EQUAL(
        std::string("value 2"),
        std::get<1>(
            hashtable->get_if_exists(
                std::hash<std::string>()("key"),
                std::bind(std::equal_to<std::string>(), "key", std::placeholders::_1))));
  }

  TEST_FIXTURE(hashtable_fixture, get_if_present_on_empty_table) {
    init_test(std::tuple<>());

    auto get_result = hashtable->get_if_exists(std::hash<std::string>()("key"), std::bind(std::equal_to<std::string>(), "key", std::placeholders::_1));
    CHECK_EQUAL(0, get_result.index());
  }

  TEST_FIXTURE(hashtable_fixture, get_if_present_finds_a_value) {
    init_test(std::tuple<>());
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key_1"), std::string("value_1"));
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key_2"), std::string("value_2"));
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key_3"), std::string("value_3"));

    auto get_result_1 = hashtable->get_if_exists(std::hash<std::string>()("key_1"), std::bind(std::equal_to<std::string>(), "key_1", std::placeholders::_1));
    CHECK_EQUAL(std::string("value_1"), std::get<1>(get_result_1));

    auto get_result_2 = hashtable->get_if_exists(std::hash<std::string>()("key_2"), std::bind(std::equal_to<std::string>(), "key_2", std::placeholders::_1));
    CHECK_EQUAL(std::string("value_2"), std::get<1>(get_result_2));

    auto get_result_3 = hashtable->get_if_exists(std::hash<std::string>()("key_3"), std::bind(std::equal_to<std::string>(), "key_3", std::placeholders::_1));
    CHECK_EQUAL(std::string("value_3"), std::get<1>(get_result_3));
  }

  TEST_FIXTURE(hashtable_fixture, expire) {
    init_test(std::tuple<>());
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key_1"), std::string("value_1"));
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key_2"), std::string("value_2"));
    hashtable->emplace(
        std::hash<std::string>(),
        std::equal_to<std::string>(),
        std::string("key_3"), std::string("value_3"));

    hashtable->expire(std::hash<std::string>()("key_3"), std::bind(std::equal_to<std::string>(), "key_3", std::placeholders::_1));

    auto get_result = hashtable->get_if_exists(std::hash<std::string>()("key_3"), std::bind(std::equal_to<std::string>(), "key_3", std::placeholders::_1));
    CHECK(get_result.index() == 0);

    // Other keys are not expired.
    auto get_result_1 = hashtable->get_if_exists(std::hash<std::string>()("key_1"), std::bind(std::equal_to<std::string>(), "key_1", std::placeholders::_1));
    CHECK_EQUAL(std::string("value_1"), std::get<1>(get_result_1));

    auto get_result_2 = hashtable->get_if_exists(std::hash<std::string>()("key_2"), std::bind(std::equal_to<std::string>(), "key_2", std::placeholders::_1));
    CHECK_EQUAL(std::string("value_2"), std::get<1>(get_result_2));
  }
}
