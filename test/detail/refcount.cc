#include <libhoard/detail/refcount.h>

#include "UnitTest++/UnitTest++.h"

SUITE(refcount) {
  class fixture {
    public:
    template<typename T, typename Allocator>
    using refcount_ptr = libhoard::detail::refcount_ptr<T, Allocator>;
    using refcount = libhoard::detail::refcount;

    struct type : refcount {};

    class test_allocator {
      public:
      using value_type = type;

      test_allocator() = default;
      test_allocator(fixture* self) : self(self) {}

      auto allocate(std::size_t allocate_count) -> type* {
        REQUIRE CHECK(self != nullptr); // If nullpointer, we're not meant to allocate.

        ++self->allocate_called;
        CHECK_EQUAL(1u, allocate_count);
        return &self->instance;
      }

      auto deallocate(type* deallocate_pointer, std::size_t deallocate_count) {
        REQUIRE CHECK(self != nullptr); // If nullpointer, we're not meant to allocate.

        ++self->deallocate_called;
        CHECK_EQUAL(1u, deallocate_count);
        CHECK_EQUAL(&self->instance, deallocate_pointer);
      }

      private:
      fixture* self = nullptr;
    };

    void reset_counters() {
      allocate_called = deallocate_called = 0;
    }

    test_allocator allocator() { return test_allocator(this); }
    std::size_t allocate_called = 0, deallocate_called = 0;
    type instance;
  };


  TEST_FIXTURE(fixture, dfl_constructor) {
    refcount_ptr<type, test_allocator> ptr;

    CHECK_EQUAL(nullptr, ptr.get());
    CHECK_EQUAL(nullptr, &ptr.operator*());
    CHECK_EQUAL(nullptr, ptr.operator->());
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(0u, deallocate_called);
  }

  TEST_FIXTURE(fixture, allocator_constructor) {
    auto ptr = refcount_ptr<type, test_allocator>(test_allocator());

    CHECK_EQUAL(nullptr, ptr.get());
    CHECK_EQUAL(nullptr, &ptr.operator*());
    CHECK_EQUAL(nullptr, ptr.operator->());
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(0u, deallocate_called);
  }

  TEST_FIXTURE(fixture, nullpointer_constructor) {
    auto ptr = refcount_ptr<type, test_allocator>(nullptr);

    CHECK_EQUAL(nullptr, ptr.get());
    CHECK_EQUAL(nullptr, &ptr.operator*());
    CHECK_EQUAL(nullptr, ptr.operator->());
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(0u, deallocate_called);
  }

  TEST_FIXTURE(fixture, nullpointer_and_allocator_constructor) {
    auto ptr = refcount_ptr<type, test_allocator>(nullptr, test_allocator());

    CHECK_EQUAL(nullptr, ptr.get());
    CHECK_EQUAL(nullptr, &ptr.operator*());
    CHECK_EQUAL(nullptr, ptr.operator->());
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(0u, deallocate_called);
  }

  TEST_FIXTURE(fixture, adopt) {
    {
      libhoard::detail::refcount_inc(&instance);
      auto ptr = refcount_ptr<type, test_allocator>(allocator());

      ptr.adopt(&instance);
      CHECK_EQUAL(&instance, ptr.get());
      CHECK_EQUAL(&instance, &ptr.operator*());
      CHECK_EQUAL(&instance, ptr.operator->());
      CHECK_EQUAL(0u, allocate_called);
      CHECK_EQUAL(0u, deallocate_called);
    }

    // Instance is deallocated when ptr is destroyed.
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(1u, deallocate_called);
  }

  TEST_FIXTURE(fixture, make_instance) {
    {
      auto ptr = libhoard::detail::allocate_refcount<type>(allocator());
      CHECK_EQUAL(&instance, ptr.get());
      CHECK_EQUAL(1u, allocate_called);
      CHECK_EQUAL(0u, deallocate_called);

      reset_counters();
    }

    // Instance is deallocated when ptr is destroyed.
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(1u, deallocate_called);
  }

  TEST_FIXTURE(fixture, reset) {
    auto ptr = libhoard::detail::allocate_refcount<type>(allocator());
    reset_counters();

    ptr.reset();
    CHECK_EQUAL(nullptr, ptr.get());
    CHECK_EQUAL(nullptr, &ptr.operator*());
    CHECK_EQUAL(nullptr, ptr.operator->());
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(1u, deallocate_called);
  }

  TEST_FIXTURE(fixture, copy_construction) {
    {
      auto ptr_1 = libhoard::detail::allocate_refcount<type>(allocator());
      reset_counters();

      auto ptr_2 = ptr_1;

      // ptr_1 retains its values
      CHECK_EQUAL(&instance, ptr_1.get());
      CHECK_EQUAL(&instance, &ptr_1.operator*());
      CHECK_EQUAL(&instance, ptr_1.operator->());

      // ptr_2 has these values
      CHECK_EQUAL(&instance, ptr_2.get());
      CHECK_EQUAL(&instance, &ptr_2.operator*());
      CHECK_EQUAL(&instance, ptr_2.operator->());

      CHECK_EQUAL(0u, allocate_called);
      CHECK_EQUAL(0u, deallocate_called);
    }

    // Destruction still happens once.
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(1u, deallocate_called);
  }

  TEST_FIXTURE(fixture, move_construction) {
    {
      auto ptr_1 = libhoard::detail::allocate_refcount<type>(allocator());
      reset_counters();

      auto ptr_2 = std::move(ptr_1);

      // ptr_1 is reset
      CHECK_EQUAL(nullptr, ptr_1.get());
      CHECK_EQUAL(nullptr, &ptr_1.operator*());
      CHECK_EQUAL(nullptr, ptr_1.operator->());

      // ptr_2 has these values
      CHECK_EQUAL(&instance, ptr_2.get());
      CHECK_EQUAL(&instance, &ptr_2.operator*());
      CHECK_EQUAL(&instance, ptr_2.operator->());

      CHECK_EQUAL(0u, allocate_called);
      CHECK_EQUAL(0u, deallocate_called);
    }

    // Destruction still happens once.
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(1u, deallocate_called);
  }

  TEST_FIXTURE(fixture, copy_assignment) {
    {
      refcount_ptr<type, test_allocator> ptr_2;
      auto ptr_1 = libhoard::detail::allocate_refcount<type>(allocator());
      reset_counters();

      ptr_2 = ptr_1;

      // ptr_1 retains its values
      CHECK_EQUAL(&instance, ptr_1.get());
      CHECK_EQUAL(&instance, &ptr_1.operator*());
      CHECK_EQUAL(&instance, ptr_1.operator->());

      // ptr_2 has these values
      CHECK_EQUAL(&instance, ptr_2.get());
      CHECK_EQUAL(&instance, &ptr_2.operator*());
      CHECK_EQUAL(&instance, ptr_2.operator->());

      CHECK_EQUAL(0u, allocate_called);
      CHECK_EQUAL(0u, deallocate_called);
    }

    // Destruction still happens once.
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(1u, deallocate_called);
  }

  TEST_FIXTURE(fixture, move_assignment) {
    {
      refcount_ptr<type, test_allocator> ptr_2;
      auto ptr_1 = libhoard::detail::allocate_refcount<type>(allocator());
      reset_counters();

      ptr_2 = std::move(ptr_1);

      // ptr_1 is reset
      CHECK_EQUAL(nullptr, ptr_1.get());
      CHECK_EQUAL(nullptr, &ptr_1.operator*());
      CHECK_EQUAL(nullptr, ptr_1.operator->());

      // ptr_2 has these values
      CHECK_EQUAL(&instance, ptr_2.get());
      CHECK_EQUAL(&instance, &ptr_2.operator*());
      CHECK_EQUAL(&instance, ptr_2.operator->());

      CHECK_EQUAL(0u, allocate_called);
      CHECK_EQUAL(0u, deallocate_called);
    }

    // Destruction still happens once.
    CHECK_EQUAL(0u, allocate_called);
    CHECK_EQUAL(1u, deallocate_called);
  }

  TEST_FIXTURE(fixture, comparison) {
    refcount_ptr<type, std::allocator<type>> ptr_1, ptr_2, ptr_1_copy, ptr_3;
    refcount_ptr<const type, std::allocator<type>> const_ptr_1;
    ptr_1_copy = ptr_1 = libhoard::detail::allocate_refcount<type>(std::allocator<type>());
    const_ptr_1 = ptr_1;
    ptr_2 = nullptr;
    ptr_3 = libhoard::detail::allocate_refcount<type>(std::allocator<type>());

    CHECK(!(ptr_1 == nullptr));
    CHECK(ptr_1 != nullptr);

    CHECK(ptr_1 == ptr_1_copy);
    CHECK(!(ptr_1 != ptr_1_copy));

    CHECK(!(ptr_1 == ptr_3));
    CHECK(ptr_1 != ptr_3);

    CHECK(const_ptr_1 == ptr_1_copy);
    CHECK(!(const_ptr_1 != ptr_1_copy));

    CHECK(!(const_ptr_1 == ptr_3));
    CHECK(const_ptr_1 != ptr_3);

    CHECK(ptr_2 == nullptr);
    CHECK(!(ptr_2 != nullptr));
  }
}
