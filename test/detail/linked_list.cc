#include <libhoard/detail/linked_list.h>

#include <algorithm>
#include <array>

#include "UnitTest++/UnitTest++.h"

using libhoard::detail::linked_list;

SUITE(linked_list) {
  class linked_list_fixture {
    public:
    struct tag;

    class element
    : public libhoard::detail::linked_list_link<tag>
    {};

    using list_type = linked_list<element, tag>;

    struct is_same_address {
      template<typename T, typename U>
      auto operator()(const T& t, const U& u) const noexcept -> bool {
        return std::addressof(t) == std::addressof(u);
      }
    };

    protected:
    list_type list;
  };

  TEST_FIXTURE(linked_list_fixture, empty_list) {
    CHECK(list.empty());
    CHECK(list.begin() == list.end());
    CHECK(list.cbegin() == list.cend());
  }

  TEST_FIXTURE(linked_list_fixture, link_front) {
    std::array<element, 3> elems;

    list.link_front(&elems[2]);
    list.link_front(&elems[1]);
    list.link_front(&elems[0]);
    CHECK(std::equal(
        elems.begin(), elems.end(),
        list.begin(), list.end(),
        is_same_address()));
    CHECK(!list.empty());
  }

  TEST_FIXTURE(linked_list_fixture, link_back) {
    std::array<element, 3> elems;

    list.link_back(&elems[0]);
    list.link_back(&elems[1]);
    list.link_back(&elems[2]);
    CHECK(std::equal(
        elems.begin(), elems.end(),
        list.begin(), list.end(),
        is_same_address()));
    CHECK(!list.empty());
  }

  TEST_FIXTURE(linked_list_fixture, link_before) {
    std::array<element, 3> elems;
    list.link_back(&elems[0]);
    list.link_back(&elems[2]);

    list.link_before(list.iterator_to(&elems[2]), &elems[1]);
    CHECK(std::equal(
        elems.begin(), elems.end(),
        list.begin(), list.end(),
        is_same_address()));
    CHECK(!list.empty());
  }

  TEST_FIXTURE(linked_list_fixture, link_after) {
    std::array<element, 3> elems;
    list.link_back(&elems[0]);
    list.link_back(&elems[2]);

    list.link_after(list.iterator_to(&elems[0]), &elems[1]);
    CHECK(std::equal(
        elems.begin(), elems.end(),
        list.begin(), list.end(),
        is_same_address()));
    CHECK(!list.empty());
  }

  TEST_FIXTURE(linked_list_fixture, unlink_first) {
    std::array<element, 2> elems;
    element to_rm;
    list.link_back(&to_rm);
    list.link_back(&elems[0]);
    list.link_back(&elems[1]);

    auto unlink_result = list.unlink(list.iterator_to(&to_rm));
    CHECK(std::equal(
        elems.begin(), elems.end(),
        list.begin(), list.end(),
        is_same_address()));
    CHECK(unlink_result == list.begin());
  }

  TEST_FIXTURE(linked_list_fixture, unlink_last) {
    std::array<element, 2> elems;
    element to_rm;
    list.link_back(&elems[0]);
    list.link_back(&elems[1]);
    list.link_back(&to_rm);

    auto unlink_result = list.unlink(list.iterator_to(&to_rm));
    CHECK(std::equal(
        elems.begin(), elems.end(),
        list.begin(), list.end(),
        is_same_address()));
    CHECK(unlink_result == list.end());
  }

  TEST_FIXTURE(linked_list_fixture, unlink_middle) {
    std::array<element, 2> elems;
    element to_rm;
    list.link_back(&elems[0]);
    list.link_back(&to_rm);
    list.link_back(&elems[1]);

    auto unlink_result = list.unlink(list.iterator_to(&to_rm));
    CHECK(std::equal(
        elems.begin(), elems.end(),
        list.begin(), list.end(),
        is_same_address()));
    CHECK(unlink_result == list.iterator_to(&elems[1]));
  }

  TEST_FIXTURE(linked_list_fixture, unlink_single_element) {
    element to_rm;
    list.link_back(&to_rm);

    auto unlink_result = list.unlink(list.iterator_to(&to_rm));
    CHECK_EQUAL(0, std::distance(list.begin(), list.end()));
    CHECK(unlink_result == list.end());
    CHECK(list.empty());
  }

  TEST_FIXTURE(linked_list_fixture, iterator_to) {
    element elem;
    const element const_elem;
    list.link_back(&elem);
    list.link_back(const_cast<element*>(&const_elem));

    auto elem_iter = list.iterator_to(&elem);
    CHECK_EQUAL(&elem, elem_iter.get());

    auto const_elem_iter = list.iterator_to(&const_elem);
    CHECK_EQUAL(&const_elem, const_elem_iter.get());
  }

  TEST_FIXTURE(linked_list_fixture, iterate) {
    std::array<element, 3> elems;
    list.link_back(&elems[0]);
    list.link_back(&elems[1]);
    list.link_back(&elems[2]);

    auto iter = list.begin();
    CHECK(&*iter == &elems[0]);
    ++iter;
    CHECK(&*iter == &elems[1]);
    ++iter;
    CHECK(&*iter == &elems[2]);
    ++iter;
    CHECK(iter == list.end());
  }

  TEST_FIXTURE(linked_list_fixture, const_iterate) {
    std::array<element, 3> elems;
    list.link_back(&elems[0]);
    list.link_back(&elems[1]);
    list.link_back(&elems[2]);

    auto iter = list.cbegin();
    CHECK(&*iter == &elems[0]);
    ++iter;
    CHECK(&*iter == &elems[1]);
    ++iter;
    CHECK(&*iter == &elems[2]);
    ++iter;
    CHECK(iter == list.cend());
  }

  TEST_FIXTURE(linked_list_fixture, iterate_backwards) {
    std::array<element, 3> elems;
    list.link_back(&elems[0]);
    list.link_back(&elems[1]);
    list.link_back(&elems[2]);

    auto iter = list.end();
    --iter;
    CHECK(&*iter == &elems[2]);
    --iter;
    CHECK(&*iter == &elems[1]);
    --iter;
    CHECK(&*iter == &elems[0]);
    CHECK(iter == list.begin());
  }

  TEST_FIXTURE(linked_list_fixture, const_iterate_backwards) {
    std::array<element, 3> elems;
    list.link_back(&elems[0]);
    list.link_back(&elems[1]);
    list.link_back(&elems[2]);

    auto iter = list.cend();
    --iter;
    CHECK(&*iter == &elems[2]);
    --iter;
    CHECK(&*iter == &elems[1]);
    --iter;
    CHECK(&*iter == &elems[0]);
    CHECK(iter == list.cbegin());
  }
}
