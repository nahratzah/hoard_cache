#include <libhoard/detail/basic_hashtable.h>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

#include "UnitTest++/UnitTest++.h"

using libhoard::detail::basic_hashtable_algorithms;
using libhoard::detail::basic_hashtable;

class algorithms_fixture {
  public:
  class bha
  : public basic_hashtable_algorithms
  {
    friend algorithms_fixture;

    public:
    template<typename... Args>
    explicit bha(Args&&... args)
    : basic_hashtable_algorithms(std::forward<Args>(args)...)
    {}

    // Expose protected members.
    auto rehash_(successor_ptr* buckets, size_type bucket_count) noexcept {
      return this->basic_hashtable_algorithms::rehash_(buckets, bucket_count);
    }

    auto release_buckets_() noexcept {
      return this->basic_hashtable_algorithms::release_buckets_();
    }

    auto link_(std::size_t hash, element* elem) noexcept {
      return this->basic_hashtable_algorithms::link_(hash, elem);
    }
  };

  class bha_element
  : public libhoard::detail::basic_hashtable_algorithms::element
  {
    friend algorithms_fixture;
  };

  struct is_same_address {
    template<typename T, typename U>
    auto operator()(const T& t, const U& u) const noexcept -> bool {
      return std::addressof(t) == std::addressof(u);
    }
  };

  protected:
  template<typename... Args>
  auto test_init(Args&&... args) -> void {
    hashtable = std::make_unique<bha>(std::forward<Args>(args)...);
  }

  std::unique_ptr<bha> hashtable;
};


class basic_hashtable_fixture {
  public:
  class bha_element
  : public libhoard::detail::basic_hashtable_algorithms::element
  {
    friend algorithms_fixture;
  };

  class bha
  : public basic_hashtable<>
  {
    friend algorithms_fixture;

    public:
    template<typename... Args>
    explicit bha(Args&&... args)
    : basic_hashtable<>(std::forward<Args>(args)...)
    {}
  };

  struct is_same_address {
    template<typename T, typename U>
    auto operator()(const T& t, const U& u) const noexcept -> bool {
      return std::addressof(t) == std::addressof(u);
    }
  };

  protected:
  template<typename... Args>
  auto test_init(Args&&... args) -> void {
    hashtable = std::make_unique<bha>(std::forward<Args>(args)...);
  }

  std::unique_ptr<bha> hashtable;
};


SUITE(basic_hashtable_algorithms) {
  // Test that default construction initializes the hashtable as intended.
  TEST_FIXTURE(algorithms_fixture, construction) {
    test_init();

    CHECK(hashtable->empty());
    CHECK_EQUAL(0u, hashtable->size());
    CHECK_EQUAL(0u, hashtable->bucket_count());
    CHECK_CLOSE(0.0, hashtable->load_factor(), 0.000001f); // Very important this isn't NaN.
  }

  // Test that rehash and release work, on an empty hashtable.
  TEST_FIXTURE(algorithms_fixture, rehash_and_release) {
    std::array<basic_hashtable_algorithms::successor_ptr, 1> buckets_1;
    std::array<basic_hashtable_algorithms::successor_ptr, 2> buckets_2;
    test_init();

    const auto rehash_result_1 = hashtable->rehash_(&buckets_1[0], buckets_1.size());
    CHECK_EQUAL(std::get<0>(rehash_result_1), nullptr);
    CHECK_EQUAL(std::get<1>(rehash_result_1), 0);
    CHECK_EQUAL(buckets_1.size(), hashtable->bucket_count());

    const auto rehash_result_2 = hashtable->rehash_(&buckets_2[0], buckets_2.size());
    CHECK_EQUAL(std::get<0>(rehash_result_2), &buckets_1[0]);
    CHECK_EQUAL(std::get<1>(rehash_result_2), buckets_1.size());
    CHECK_EQUAL(buckets_2.size(), hashtable->bucket_count());

    const auto release_result = hashtable->release_buckets_();
    CHECK_EQUAL(std::get<0>(release_result), &buckets_2[0]);
    CHECK_EQUAL(std::get<1>(release_result), buckets_2.size());
  }

  // Test that we can insert an element.
  TEST_FIXTURE(algorithms_fixture, link) {
    bha_element elem;
    std::array<basic_hashtable_algorithms::successor_ptr, 1> buckets_1;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());

    const auto iter = hashtable->link_(0, &elem);
    CHECK(iter == hashtable->before_begin()); // link returns the predecessor
    CHECK(!hashtable->empty());
    CHECK_EQUAL(1u, hashtable->size());
    CHECK_CLOSE(1.0, hashtable->load_factor(), 0.000001f);
    CHECK_EQUAL(&elem, &*hashtable->begin());
    CHECK(++hashtable->begin() == hashtable->end());
    CHECK_EQUAL(&elem, &*hashtable->before_end());
    CHECK(++hashtable->before_begin() == hashtable->before_end());
    CHECK_EQUAL(0u, elem.hash());
  }

  // Test that linking multiple elements into the same bucket, maintains insertion order.
  TEST_FIXTURE(algorithms_fixture, link_is_stable) {
    std::array<bha_element, 3> elem;
    std::array<basic_hashtable_algorithms::successor_ptr, 1> buckets_1;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());

    const auto iter_0 = ++hashtable->link_(0, &elem[0]);
    const auto iter_1 = ++hashtable->link_(0, &elem[1]);
    const auto iter_2 = ++hashtable->link_(0, &elem[2]);

    auto iter = hashtable->begin(); // elem[0]
    CHECK(iter == iter_0);
    CHECK(&*iter == &elem[0]);

    ++iter; // elem[1]
    CHECK(iter == iter_1);
    CHECK(&*iter == &elem[1]);

    ++iter; // elem[2]
    CHECK(iter == iter_2);
    CHECK(&*iter == &elem[2]);
  }

  // Test that rehash keeps the elements in the hashtable.
  TEST_FIXTURE(algorithms_fixture, rehash_with_elements) {
    std::array<bha_element, 3> elems;
    std::array<basic_hashtable_algorithms::successor_ptr, 1> buckets_1;
    std::array<basic_hashtable_algorithms::successor_ptr, 3> rehashed_buckets;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());
    hashtable->link_(0, &elems[0]);
    hashtable->link_(1, &elems[1]);
    hashtable->link_(2, &elems[2]);

    hashtable->rehash_(&rehashed_buckets[0], rehashed_buckets.size());
    CHECK(std::is_permutation(
        elems.begin(), elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));
  }

  // Test that rehashing doesn't affect insertion order
  // for elements with the same hash code.
  TEST_FIXTURE(algorithms_fixture, rehash_is_stable) {
    std::array<bha_element, 3> elems;
    std::array<basic_hashtable_algorithms::successor_ptr, 1> buckets_1;
    std::array<basic_hashtable_algorithms::successor_ptr, 3> rehashed_buckets;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());
    hashtable->link_(0, &elems[0]);
    hashtable->link_(0, &elems[1]);
    hashtable->link_(0, &elems[2]);

    hashtable->rehash_(&rehashed_buckets[0], rehashed_buckets.size());
    CHECK(std::equal(
        elems.begin(), elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));
  }

  TEST_FIXTURE(algorithms_fixture, unlink) {
    std::array<bha_element, 3> elems;
    std::array<basic_hashtable_algorithms::successor_ptr, 1> buckets_1;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());
    hashtable->link_(0, &elems[0]);
    hashtable->link_(0, &elems[1]);
    hashtable->link_(0, &elems[2]);
    // Test needs elements in this order.
    REQUIRE CHECK(std::equal(
        elems.begin(), elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));

    hashtable->unlink(hashtable->before_begin()); // unlink elems[0]
    CHECK(std::equal(
        elems.begin() + 1, elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));

    hashtable->unlink(++hashtable->before_begin()); // unlink elems[2]
    CHECK(std::equal(
        elems.begin() + 1, elems.end() - 1,
        hashtable->begin(), hashtable->end(),
        is_same_address()));

    hashtable->unlink(hashtable->before_begin()); // unlink elems[1]
    CHECK(hashtable->empty());
    CHECK(hashtable->begin() == hashtable->end());
  }

  TEST_FIXTURE(algorithms_fixture, hash_to_bucket_selection) {
    std::array<bha_element, 3> elems;
    std::array<basic_hashtable_algorithms::successor_ptr, 3> buckets_1;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());
    hashtable->link_(0, &elems[0]);
    hashtable->link_(1, &elems[1]);
    hashtable->link_(2, &elems[2]);

    CHECK_EQUAL(0u, hashtable->bucket_for(std::size_t(0u)));
    CHECK_EQUAL(1u, hashtable->bucket_for(std::size_t(1u)));
    CHECK_EQUAL(2u, hashtable->bucket_for(std::size_t(2u)));

    CHECK_EQUAL(0u, hashtable->bucket_for(&elems[0]));
    CHECK_EQUAL(1u, hashtable->bucket_for(&elems[1]));
    CHECK_EQUAL(2u, hashtable->bucket_for(&elems[2]));
  }

  TEST_FIXTURE(algorithms_fixture, iteration) {
    std::array<bha_element, 6> elems;
    std::array<basic_hashtable_algorithms::successor_ptr, 3> buckets_1;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());
    hashtable->link_(0, &elems[0]);
    hashtable->link_(0, &elems[1]);
    hashtable->link_(1, &elems[2]);
    hashtable->link_(1, &elems[3]);
    hashtable->link_(2, &elems[4]);
    hashtable->link_(2, &elems[5]);

    CHECK(std::equal(
        elems.begin(), elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));
    CHECK(std::equal(
        elems.begin() + 0, elems.begin() + 2,
        hashtable->begin(0), hashtable->end(0),
        is_same_address()));
    CHECK(std::equal(
        elems.begin() + 2, elems.begin() + 4,
        hashtable->begin(1), hashtable->end(1),
        is_same_address()));
    CHECK(std::equal(
        elems.begin() + 4, elems.begin() + 6,
        hashtable->begin(2), hashtable->end(2),
        is_same_address()));
  }

  TEST_FIXTURE(algorithms_fixture, const_iteration) {
    std::array<bha_element, 6> elems;
    std::array<basic_hashtable_algorithms::successor_ptr, 3> buckets_1;
    test_init();
    hashtable->rehash_(&buckets_1[0], buckets_1.size());
    hashtable->link_(0, &elems[0]);
    hashtable->link_(0, &elems[1]);
    hashtable->link_(1, &elems[2]);
    hashtable->link_(1, &elems[3]);
    hashtable->link_(2, &elems[4]);
    hashtable->link_(2, &elems[5]);

    CHECK(std::equal(
        elems.cbegin(), elems.cend(),
        hashtable->cbegin(), hashtable->cend(),
        is_same_address()));
    CHECK(std::equal(
        elems.begin() + 0, elems.begin() + 2,
        hashtable->cbegin(0), hashtable->cend(0),
        is_same_address()));
    CHECK(std::equal(
        elems.begin() + 2, elems.begin() + 4,
        hashtable->cbegin(1), hashtable->cend(1),
        is_same_address()));
    CHECK(std::equal(
        elems.begin() + 4, elems.begin() + 6,
        hashtable->cbegin(2), hashtable->cend(2),
        is_same_address()));
  }
}


SUITE(basic_hashtable) {
  // Test that default construction initializes the hashtable as intended.
  TEST_FIXTURE(basic_hashtable_fixture, construction) {
    test_init();

    CHECK(hashtable->empty());
    CHECK_EQUAL(0u, hashtable->size());
    CHECK_EQUAL(0u, hashtable->bucket_count());
    CHECK_CLOSE(0.0, hashtable->load_factor(), 0.000001f); // Very important this isn't NaN.
    CHECK_CLOSE(1.0, hashtable->max_load_factor(), 0.000001f);
  }

  // Test that construction with a specific max-load-factor works as intended.
  TEST_FIXTURE(basic_hashtable_fixture, max_load_factor_construction) {
    test_init(17.5f);

    CHECK(hashtable->empty());
    CHECK_EQUAL(0u, hashtable->size());
    CHECK_EQUAL(0u, hashtable->bucket_count());
    CHECK_CLOSE(0.0, hashtable->load_factor(), 0.000001f); // Very important this isn't NaN.
    CHECK_CLOSE(17.5f, hashtable->max_load_factor(), 0.000001f);
    CHECK_EQUAL(std::allocator_traits<bha::allocator_type>::max_size(hashtable->get_allocator()), hashtable->max_bucket_count());
  }

  TEST_FIXTURE(basic_hashtable_fixture, reserve) {
    test_init(1.0f);
    hashtable->reserve(1000);
    CHECK_EQUAL(1000, hashtable->bucket_count());

    test_init(0.5f);
    hashtable->reserve(1000);
    CHECK_EQUAL(2000, hashtable->bucket_count());
  }

  TEST_FIXTURE(basic_hashtable_fixture, link) {
    std::array<bha_element, 6> elems;
    test_init();
    hashtable->link(0, &elems[0]);
    hashtable->link(1, &elems[1]);
    hashtable->link(2, &elems[2]);
    hashtable->link(3, &elems[3]);
    hashtable->link(4, &elems[4]);
    hashtable->link(5, &elems[5]);

    CHECK(std::is_permutation(
        elems.begin(), elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));
    CHECK(hashtable->load_factor() <= hashtable->max_load_factor());
  }

  TEST_FIXTURE(basic_hashtable_fixture, link_respects_max_load_factor) {
    std::array<bha_element, 6> elems;
    test_init(0.1f);
    hashtable->link(0, &elems[0]);
    hashtable->link(1, &elems[1]);
    hashtable->link(2, &elems[2]);
    hashtable->link(3, &elems[3]);
    hashtable->link(4, &elems[4]);
    hashtable->link(5, &elems[5]);

    CHECK(std::is_permutation(
        elems.begin(), elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));
    CHECK(hashtable->load_factor() <= hashtable->max_load_factor());
  }

  TEST_FIXTURE(basic_hashtable_fixture, link_only_rehashes_when_needed) {
    std::array<bha_element, 6> elems;
    test_init(10.0f);
    hashtable->link(0, &elems[0]);
    REQUIRE CHECK(hashtable->bucket_count() < 10);
    const auto num_buckets = hashtable->bucket_count();

    hashtable->link(1, &elems[1]);
    hashtable->link(2, &elems[2]);
    hashtable->link(3, &elems[3]);
    hashtable->link(4, &elems[4]);
    hashtable->link(5, &elems[5]);

    CHECK(std::is_permutation(
        elems.begin(), elems.end(),
        hashtable->begin(), hashtable->end(),
        is_same_address()));
    CHECK(hashtable->load_factor() <= hashtable->max_load_factor());
    CHECK_EQUAL(num_buckets, hashtable->bucket_count());
  }

  TEST_FIXTURE(basic_hashtable_fixture, reserve_runs_callback_before_rehash) {
    test_init();
    const auto expected_bucket_count = hashtable->bucket_count();
    bool callback_was_called = false;

    hashtable->reserve(1000, [this, &callback_was_called, expected_bucket_count]() {
          callback_was_called = true;
          CHECK_EQUAL(expected_bucket_count, hashtable->bucket_count());
        });
    CHECK(callback_was_called);
  }

  TEST_FIXTURE(basic_hashtable_fixture, link_runs_callback_before_rehash) {
    bha_element elem;
    test_init();
    const auto expected_bucket_count = hashtable->bucket_count();
    bool callback_was_called = false;

    hashtable->link(1000, &elem, [this, &callback_was_called, expected_bucket_count]() {
          callback_was_called = true;
          CHECK_EQUAL(expected_bucket_count, hashtable->bucket_count());
        });
    CHECK(callback_was_called);
  }
}
