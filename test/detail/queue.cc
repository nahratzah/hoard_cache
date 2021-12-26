#include <libhoard/detail/queue.h>

#include <array>
#include <memory>
#include <tuple>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/meta.h>
#include <libhoard/policies.h>

using libhoard::detail::queue_policy;

template<bool EnableWeakenPolicy = false>
class queue_fixture_impl {
  public:
  class element
  : public queue_policy::value_base
  {
    using queue_policy::value_base::value_base;

    public:
    template<typename HashTable>
    explicit element(const HashTable& table, bool strong = true)
    : queue_policy::value_base(table),
      strong(strong)
    {}

    auto strengthen() {
      strong = true;
    }

    auto weaken() {
      strong = false;
    }

    auto mark_expired() -> void {
      expired = true;
    }

    bool strong;
    bool expired = false;
  };

  class impl
  : public libhoard::detail::queue<impl>
  {
    public:
    using value_type = element;
    using policy_type_list = std::conditional_t<
        EnableWeakenPolicy,
        libhoard::detail::type_list<libhoard::weaken_policy>,
        libhoard::detail::type_list<>>;

    impl()
    : libhoard::detail::queue<impl>(std::make_tuple(), std::allocator<int>()) // Queue takes arguments, but doesn't use them.
    {}

    auto lru_expire_(std::size_t count) noexcept {
      return this->libhoard::detail::queue<impl>::lru_expire_(count);
    }
  };

  protected:
  auto init_test() -> void {
    queue = std::make_unique<impl>();
  }

  std::unique_ptr<impl> queue;
};

using queue_fixture = queue_fixture_impl<false>;
using weaken_queue_fixture = queue_fixture_impl<true>;


SUITE(queue) {
  TEST_FIXTURE(queue_fixture, construct_empty_queue) {
    init_test();
    CHECK(queue->invariant());
  }

  TEST_FIXTURE(queue_fixture, on_create) {
    init_test();
    std::array<element, 3> elems{
      element(*queue, false),
      element(*queue, false),
      element(*queue, false),
    };
    REQUIRE CHECK(queue->invariant());

    queue->on_create_(&elems[0]);
    CHECK(queue->invariant());
    CHECK(elems[0].strong == false); // on_create_ doesn't call strengthen()

    queue->on_create_(&elems[1]);
    CHECK(queue->invariant());
    CHECK(elems[1].strong == false); // on_create_ doesn't call strengthen()

    queue->on_create_(&elems[2]);
    CHECK(queue->invariant());
    CHECK(elems[2].strong == false); // on_create_ doesn't call strengthen()
  }

  TEST_FIXTURE(queue_fixture, on_create_strengthens_hot_elements) {
    init_test();
    std::array<element, 2> elems{
      element(*queue, false),
      element(*queue, false),
    };
    queue->on_create_(&elems[0]);
    queue->on_create_(&elems[1]);

    // Since elems[1] is created last, it must be cold.
    // But because the queue always balances the hot and cold sides,
    // this means elems[0] has to be hot.
    CHECK(elems[0].strong);
    CHECK(!elems[1].strong);
  }

  TEST_FIXTURE(queue_fixture, on_hit) {
    init_test();
    std::array<element, 3> elems{
      element(*queue),
      element(*queue),
      element(*queue),
    };
    queue->on_create_(&elems[0]);
    queue->on_create_(&elems[1]);
    queue->on_create_(&elems[2]);
    elems[0].strong = elems[1].strong = elems[2].strong = false;
    REQUIRE CHECK(queue->invariant());

    queue->on_hit_(&elems[2]);
    CHECK(elems[2].strong); // on_hit_ calls strengthen()
  }

  TEST_FIXTURE(queue_fixture, on_hit_does_strengthen) {
    init_test();
    std::array<element, 3> elems{
      element(*queue),
      element(*queue),
      element(*queue),
    };
    queue->on_create_(&elems[0]);
    queue->on_create_(&elems[1]);
    queue->on_create_(&elems[2]);
    elems[0].strong = elems[1].strong = elems[2].strong = false;
    REQUIRE CHECK(queue->invariant());

    queue->on_hit_(&elems[0]);
    queue->on_hit_(&elems[1]);
    queue->on_hit_(&elems[2]);
    CHECK(elems[0].strong);
    CHECK(elems[1].strong);
    CHECK(elems[2].strong);
  }

  TEST_FIXTURE(queue_fixture, lru_expire) {
    init_test();
    std::array<element, 2> elems{
      element(*queue, false),
      element(*queue, false),
    };
    queue->on_create_(&elems[0]);
    queue->on_create_(&elems[1]);
    // Due to how the queue works, the first element will be hot, the second will be cold.
    REQUIRE CHECK(elems[0].strong);
    REQUIRE CHECK(!elems[1].strong);

    queue->lru_expire_(5);
    CHECK(!elems[0].expired); // Never expire hot elements.
    CHECK(elems[1].expired); // Expire cold elements.
  }

  TEST_FIXTURE(weaken_queue_fixture, lru_expire_does_weaken_if_policy_present) {
    init_test();
    std::array<element, 2> elems{
      element(*queue, false),
      element(*queue, false),
    };
    queue->on_create_(&elems[0]);
    queue->on_create_(&elems[1]);
    // Due to how the queue works, the first element will be hot, the second will be cold.
    REQUIRE CHECK(elems[0].strong);
    REQUIRE CHECK(!elems[1].strong);

    queue->lru_expire_(5);
    CHECK(elems[0].strong); // Never expire hot elements.
    CHECK(!elems[1].strong); // Expire cold elements.

    // When we weaken elements, we must not expire them.
    CHECK(!elems[0].expired);
    CHECK(!elems[1].expired);
  }

  TEST_FIXTURE(queue_fixture, unlinking) {
    init_test();
    std::array<element, 8> elems{
      element(*queue),
      element(*queue),
      element(*queue),
      element(*queue),
      element(*queue),
      element(*queue),
      element(*queue),
      element(*queue),
    };
    for (element& e : elems) queue->on_create_(&e);
    CHECK(queue->invariant());

    for (element& e : elems) {
      queue->on_unlink_(&e);
      CHECK(queue->invariant());
    }
  }
}
