#include <libhoard/detail/mapped_type.h>

#include <memory>
#include <string>
#include <utility>

#include "UnitTest++/UnitTest++.h"

class mapped_value_fixture {
  public:
  using mapped_value = libhoard::detail::mapped_value<std::string, std::allocator<int>, std::error_code>;

  template<typename... Args>
  auto init_test(Args&&... args) -> void {
    value = std::make_unique<mapped_value>(std::forward<Args>(args)...);
  }

  std::unique_ptr<mapped_value> value;
};


class mapped_pointer_fixture {
  public:
  using mapped_value = libhoard::detail::mapped_pointer<std::shared_ptr<std::string>, std::allocator<int>, std::error_code>;

  std::shared_ptr<std::string> pointer = std::make_shared<std::string>("bla");

  template<typename... Args>
  auto init_test(Args&&... args) -> void {
    value = std::make_unique<mapped_value>(std::forward<Args>(args)...);
  }

  std::unique_ptr<mapped_value> value;
};


SUITE(mapped_value) {
  TEST_FIXTURE(mapped_value_fixture, dfl_constructed) {
    init_test();

    CHECK(value->pending());
    CHECK(!value->expired());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());

    CHECK(value->get(std::true_type()).index() == 3);
    CHECK(value->get(std::false_type()).index() == 0);
    CHECK(value->get_pending() != nullptr);
  }

  TEST_FIXTURE(mapped_value_fixture, value_constructed) {
    init_test(std::piecewise_construct, std::make_tuple("bla"));

    CHECK(!value->pending());
    CHECK(!value->expired());
    CHECK(!value->holds_error());
    CHECK(value->holds_value());

    CHECK_EQUAL("bla", std::get<1>(value->get(std::true_type())));
    CHECK_EQUAL("bla", std::get<1>(value->get(std::false_type())));
    CHECK(value->get_pending() == nullptr);
  }

  TEST_FIXTURE(mapped_value_fixture, error_constructed) {
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    CHECK(!value->pending());
    CHECK(!value->expired());
    CHECK(value->holds_error());
    CHECK(!value->holds_value());

    CHECK_EQUAL(std::make_error_code(std::errc::connection_aborted), std::get<2>(value->get(std::true_type())));
    CHECK_EQUAL(std::make_error_code(std::errc::connection_aborted), std::get<2>(value->get(std::false_type())));
    CHECK(value->get_pending() == nullptr);
  }

  TEST_FIXTURE(mapped_value_fixture, pending_assign) {
    bool callback_called = false;
    init_test();
    value->get_pending()->add_callback([&callback_called](const std::string& v, std::error_code ex) {
          CHECK(!ex);
          CHECK_EQUAL("bla", v);
          callback_called = true;
        });

    value->assign("bla");
    CHECK(callback_called);
    CHECK(!value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<1>, "bla");
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<1>, "bla");
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, pending_assign_when_expired) {
    bool callback_called = false;
    init_test();
    value->mark_expired();
    value->get_pending()->add_callback([&callback_called](const std::string& v, std::error_code ex) {
          CHECK(!ex);
          CHECK_EQUAL("bla", v);
          callback_called = true;
        });

    value->assign("bla");
    CHECK(callback_called);
    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, pending_assign_when_weakened) {
    bool callback_called = false;
    init_test();
    value->weaken(); // Note: mapped_value weaken is the same as expiring.
    value->get_pending()->add_callback([&callback_called](const std::string& v, std::error_code ex) {
          CHECK(!ex);
          CHECK_EQUAL("bla", v);
          callback_called = true;
        });

    value->assign("bla");
    CHECK(callback_called);
    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, pending_assign_error) {
    bool callback_called = false;
    init_test();
    value->get_pending()->add_callback([&callback_called](const std::string& v, std::error_code ex) {
          CHECK(!!ex);
          CHECK_EQUAL(std::string(), v);
          callback_called = true;
        });

    value->assign_error(std::make_error_code(std::errc::connection_aborted));
    CHECK(callback_called);
    CHECK(!value->expired());
    CHECK(!value->pending());
    CHECK(value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<2>, std::make_error_code(std::errc::connection_aborted));
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<2>, std::make_error_code(std::errc::connection_aborted));
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, pending_assign_error_when_expired) {
    bool callback_called = false;
    init_test();
    value->mark_expired();
    value->get_pending()->add_callback([&callback_called](const std::string& v, std::error_code ex) {
          CHECK(!!ex);
          CHECK_EQUAL(std::string(), v);
          callback_called = true;
        });

    value->assign_error(std::make_error_code(std::errc::connection_aborted));
    CHECK(callback_called);
    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, pending_assign_error_when_weakened) {
    bool callback_called = false;
    init_test();
    value->weaken(); // Note: mapped_value weaken is the same as expiring.
    value->get_pending()->add_callback([&callback_called](const std::string& v, std::error_code ex) {
          CHECK(!!ex);
          CHECK_EQUAL(std::string(), v);
          callback_called = true;
        });

    value->assign_error(std::make_error_code(std::errc::connection_aborted));
    CHECK(callback_called);
    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, with_value_weaken) {
    init_test(std::piecewise_construct, std::make_tuple("bla"));

    value->weaken();
    CHECK(value->expired()); // Weaken on non-pointer type causes expiration.
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, with_value_expire) {
    init_test(std::piecewise_construct, std::make_tuple("bla"));

    value->mark_expired();
    CHECK(value->expired()); // Weaken on non-pointer type causes expiration.
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, with_error_weaken) {
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    value->weaken();
    CHECK(value->expired()); // Weaken on non-pointer type causes expiration.
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, with_error_expire) {
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    value->mark_expired();
    CHECK(value->expired()); // Weaken on non-pointer type causes expiration.
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_value_fixture, get_if_matching_with_matching_value) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<1>, "bla");
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple("bla"));

    auto result = value->get_if_matching(
        [&matcher_invocation_count](const std::string& v) -> bool {
          CHECK_EQUAL(std::string("bla"), v);
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(1, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_value_fixture, get_if_matching_when_value_does_not_match) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<0>);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple("bla"));

    auto result = value->get_if_matching(
        [&matcher_invocation_count](const std::string& v) -> bool {
          CHECK_EQUAL(std::string("bla"), v);
          ++matcher_invocation_count;
          return false;
        });
    CHECK_EQUAL(1, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_value_fixture, get_if_matching_with_matching_value_while_weakened) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<0>);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple("bla"));
    value->weaken();

    auto result = value->get_if_matching(
        [&matcher_invocation_count](const std::string& v) -> bool {
          CHECK_EQUAL(std::string("bla"), v);
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(0, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_value_fixture, get_if_matching_while_expired) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<0>);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple("bla"));
    value->mark_expired();

    auto result = value->get_if_matching(
        [&matcher_invocation_count](const std::string& v) -> bool {
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(0, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_value_fixture, get_if_matching_with_error) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<0>);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    auto result = value->get_if_matching(
        [&matcher_invocation_count](const std::string& v) -> bool {
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(0, matcher_invocation_count);
    CHECK(result == expected);
  }
}


SUITE(mapped_pointer) {
  TEST_FIXTURE(mapped_pointer_fixture, dfl_constructed) {
    init_test();

    CHECK(value->pending());
    CHECK(!value->expired());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());

    CHECK(value->get(std::true_type()).index() == 3);
    CHECK(value->get(std::false_type()).index() == 0);
    CHECK(value->get_pending() != nullptr);
  }

  TEST_FIXTURE(mapped_pointer_fixture, value_constructed) {
    init_test(std::piecewise_construct, std::make_tuple(pointer));

    CHECK(!value->pending());
    CHECK(!value->expired());
    CHECK(!value->holds_error());
    CHECK(value->holds_value());

    CHECK_EQUAL(pointer, std::get<1>(value->get(std::true_type())));
    CHECK_EQUAL(pointer, std::get<1>(value->get(std::false_type())));
    CHECK(value->get_pending() == nullptr);
  }

  TEST_FIXTURE(mapped_pointer_fixture, error_constructed) {
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    CHECK(!value->pending());
    CHECK(!value->expired());
    CHECK(value->holds_error());
    CHECK(!value->holds_value());

    CHECK_EQUAL(std::make_error_code(std::errc::connection_aborted), std::get<2>(value->get(std::true_type())));
    CHECK_EQUAL(std::make_error_code(std::errc::connection_aborted), std::get<2>(value->get(std::false_type())));
    CHECK(value->get_pending() == nullptr);
  }

  TEST_FIXTURE(mapped_pointer_fixture, pending_assign) {
    bool callback_called = false;
    init_test();
    value->get_pending()->add_callback([&callback_called, this](const std::shared_ptr<std::string>& v, std::error_code ex) {
          CHECK(!ex);
          CHECK_EQUAL(this->pointer, v);
          callback_called = true;
        });

    value->assign(pointer);
    CHECK(callback_called);
    CHECK(!value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<1>, pointer);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<1>, pointer);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, pending_assign_when_expired) {
    bool callback_called = false;
    init_test();
    value->mark_expired();
    value->get_pending()->add_callback([&callback_called, this](const std::shared_ptr<std::string>& v, std::error_code ex) {
          CHECK(!ex);
          CHECK_EQUAL(this->pointer, v);
          callback_called = true;
        });

    value->assign(pointer);
    CHECK(callback_called);
    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, pending_assign_when_weakened) {
    bool callback_called = false;
    init_test();
    value->weaken(); // Note: mapped_value weaken is the same as expiring.
    value->get_pending()->add_callback([&callback_called, this](const std::shared_ptr<std::string>& v, std::error_code ex) {
          CHECK(!ex);
          CHECK_EQUAL(this->pointer, v);
          callback_called = true;
        });

    value->assign(pointer);
    CHECK(callback_called);
    CHECK(!value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<1>, pointer);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<1>, pointer);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  // When the pointer is assigned in the weakened state,
  // the pointer will be lost once the last reference goes away.
  TEST_FIXTURE(mapped_pointer_fixture, pending_assign_when_weakened_may_expire) {
    init_test();
    value->weaken(); // Note: mapped_value weaken is the same as expiring.

    value->assign(pointer);
    pointer.reset(); // Last reference went away.

    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, pending_assign_error) {
    bool callback_called = false;
    init_test();
    value->get_pending()->add_callback([&callback_called](const std::shared_ptr<std::string>& v, std::error_code ex) {
          CHECK_EQUAL(std::make_error_code(std::errc::connection_aborted), ex);
          CHECK_EQUAL(nullptr, v);
          callback_called = true;
        });

    value->assign_error(std::make_error_code(std::errc::connection_aborted));
    CHECK(callback_called);
    CHECK(!value->expired());
    CHECK(!value->pending());
    CHECK(value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<2>, std::make_error_code(std::errc::connection_aborted));
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<2>, std::make_error_code(std::errc::connection_aborted));
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, pending_assign_error_when_expired) {
    bool callback_called = false;
    init_test();
    value->mark_expired();
    value->get_pending()->add_callback([&callback_called](const std::shared_ptr<std::string>& v, std::error_code ex) {
          CHECK_EQUAL(std::make_error_code(std::errc::connection_aborted), ex);
          CHECK_EQUAL(nullptr, v);
          callback_called = true;
        });

    value->assign_error(std::make_error_code(std::errc::connection_aborted));
    CHECK(callback_called);
    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, pending_assign_error_when_weakened) {
    bool callback_called = false;
    init_test();
    value->weaken(); // Note: mapped_value weaken is the same as expiring.
    value->get_pending()->add_callback([&callback_called](const std::shared_ptr<std::string>& v, std::error_code ex) {
          CHECK_EQUAL(std::make_error_code(std::errc::connection_aborted), ex);
          CHECK_EQUAL(nullptr, v);
          callback_called = true;
        });

    value->assign_error(std::make_error_code(std::errc::connection_aborted));
    CHECK(callback_called);
    CHECK(value->expired());
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, with_value_weaken) {
    init_test(std::piecewise_construct, std::make_tuple(pointer));

    value->weaken();
    CHECK(!value->expired()); // not expiring because 'pointer' keeps the value live
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<1>, pointer);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<1>, pointer);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, with_value_weaken_expires_when_last_reference_goes_away) {
    init_test(std::piecewise_construct, std::make_tuple(pointer));

    value->weaken();
    pointer.reset(); // Last reference goes away.
    CHECK(value->expired()); // not expiring because 'pointer' keeps the value live
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, with_value_expire) {
    init_test(std::piecewise_construct, std::make_tuple(pointer));

    value->mark_expired();
    CHECK(value->expired()); // Weaken on non-pointer type causes expiration.
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, with_error_weaken) {
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    value->weaken();
    CHECK(value->expired()); // Weaken on non-pointer type causes expiration.
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, with_error_expire) {
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    value->mark_expired();
    CHECK(value->expired()); // Weaken on non-pointer type causes expiration.
    CHECK(!value->pending());
    CHECK(!value->holds_error());
    CHECK(!value->holds_value());
    CHECK(value->get_pending() == nullptr);

    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expect_get_false(std::in_place_index<0>);
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type, mapped_value::pending_type*> expect_get_true(std::in_place_index<0>);
    CHECK(value->get(std::false_type()) == expect_get_false);
    CHECK(value->get(std::true_type()) == expect_get_true);
  }

  TEST_FIXTURE(mapped_pointer_fixture, get_if_matching_with_matching_value) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<1>, pointer);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple(pointer));

    auto result = value->get_if_matching(
        [&matcher_invocation_count, this](const std::shared_ptr<std::string>& v) -> bool {
          CHECK_EQUAL(this->pointer, v);
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(1, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_pointer_fixture, get_if_matching_when_value_does_not_match) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<0>);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple(pointer));

    auto result = value->get_if_matching(
        [&matcher_invocation_count, this](const std::shared_ptr<std::string>& v) -> bool {
          CHECK_EQUAL(this->pointer, v);
          ++matcher_invocation_count;
          return false;
        });
    CHECK_EQUAL(1, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_pointer_fixture, get_if_matching_with_matching_value_while_weakened) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<1>, pointer);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple(pointer));
    value->weaken();

    auto result = value->get_if_matching(
        [&matcher_invocation_count, this](const std::shared_ptr<std::string>& v) -> bool {
          CHECK_EQUAL(this->pointer, v);
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(1, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_pointer_fixture, get_if_matching_while_expired) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<0>);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_tuple(pointer));
    value->mark_expired();

    auto result = value->get_if_matching(
        [&matcher_invocation_count, this](const std::shared_ptr<std::string>& v) -> bool {
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(0, matcher_invocation_count);
    CHECK(result == expected);
  }

  TEST_FIXTURE(mapped_pointer_fixture, get_if_matching_with_error) {
    const std::variant<std::monostate, mapped_value::mapped_type, mapped_value::error_type> expected(std::in_place_index<0>);
    int matcher_invocation_count = 0;
    init_test(std::piecewise_construct, std::make_error_code(std::errc::connection_aborted));

    auto result = value->get_if_matching(
        [&matcher_invocation_count, this](const std::shared_ptr<std::string>& v) -> bool {
          ++matcher_invocation_count;
          return true;
        });
    CHECK_EQUAL(0, matcher_invocation_count);
    CHECK(result == expected);
  }
}
