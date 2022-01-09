#include <libhoard/detail/pending.h>

#include <memory>
#include <utility>
#include <system_error>

#include "UnitTest++/UnitTest++.h"

using libhoard::detail::pending;

class pending_fixture {
  public:
  using pending_type = libhoard::detail::pending<std::string, std::allocator<int>, std::error_code>;

  protected:
  template<typename... Args>
  auto init_test(Args&&... args) -> void {
    pending = std::make_unique<pending_type>(std::forward<Args>(args)...);
  }

  std::unique_ptr<pending_type> pending;
};

SUITE(pending) {
  TEST_FIXTURE(pending_fixture, constructor) {
    init_test(std::allocator<int>());

    CHECK(!pending->expired());
    CHECK(!pending->weakened());
  }

  TEST_FIXTURE(pending_fixture, weaken) {
    init_test();

    pending->weaken();
    CHECK(pending->weakened());
  }

  TEST_FIXTURE(pending_fixture, strengthen) {
    init_test();

    // Strengthening a weakened "pending".
    pending->weaken();
    bool strengthen_result_1 = pending->strengthen();
    CHECK(strengthen_result_1);
    CHECK(!pending->weakened());

    // Strengthen does the same thing irrespective of if it is weakened or not.
    bool strengthen_result_2 = pending->strengthen();
    CHECK(strengthen_result_2);
    CHECK(!pending->weakened());
  }

  TEST_FIXTURE(pending_fixture, mark_expired) {
    init_test();
    CHECK(!pending->expired());

    pending->mark_expired();
    CHECK(pending->expired());
  }

  TEST_FIXTURE(pending_fixture, resolve_success) {
    init_test();
    int count_callback_called = 0;
    pending->add_callback([&count_callback_called](const std::string& v, std::error_code ex) {
          CHECK(!ex);
          CHECK_EQUAL(R"(yay \o/)", v);
          ++count_callback_called;
        });

    pending->resolve_success(R"(yay \o/)");
    CHECK_EQUAL(1, count_callback_called);
  }

  TEST_FIXTURE(pending_fixture, resolve_failure) {
    init_test();
    int count_callback_called = 0;
    pending->add_callback([&count_callback_called](const std::string& v, std::error_code ex) {
          CHECK_EQUAL(std::make_error_code(std::errc::not_supported), ex);
          CHECK_EQUAL(std::string(), v);
          ++count_callback_called;
        });

    pending->resolve_failure(std::make_error_code(std::errc::not_supported));
    CHECK_EQUAL(1, count_callback_called);
  }

  TEST_FIXTURE(pending_fixture, cancel) {
    init_test();
    int count_callback_called = 0;
    pending->add_callback([&count_callback_called]([[maybe_unused]] const std::string& v, [[maybe_unused]] std::error_code ex) {
          ++count_callback_called;
        });

    // Cancel doesn't call any callbacks.
    pending->cancel();
    CHECK_EQUAL(0, count_callback_called);
    // Destructor doesn't call any callbacks.
    pending.reset();
    CHECK_EQUAL(0, count_callback_called);
  }

  TEST_FIXTURE(pending_fixture, canceled_pending_does_not_call_success_callbacks) {
    init_test();
    int count_callback_called = 0;
    pending->add_callback([&count_callback_called]([[maybe_unused]] const std::string& v, [[maybe_unused]] std::error_code ex) {
          ++count_callback_called;
        });

    // Cancel doesn't call any callbacks.
    pending->cancel();
    pending->resolve_success("oops");
    CHECK_EQUAL(0, count_callback_called);
  }

  TEST_FIXTURE(pending_fixture, canceled_pending_does_not_call_failure_callbacks) {
    init_test();
    int count_callback_called = 0;
    pending->add_callback([&count_callback_called]([[maybe_unused]] const std::string& v, [[maybe_unused]] std::error_code ex) {
          ++count_callback_called;
        });

    // Cancel doesn't call any callbacks.
    pending->cancel();
    pending->resolve_failure(std::make_error_code(std::errc::not_supported));
    CHECK_EQUAL(0, count_callback_called);
  }
}
