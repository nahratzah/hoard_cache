#include <libhoard/asio/resolver_policy.h>

#include <exception>
#include <string>

#include "UnitTest++/UnitTest++.h"

#include <asio/io_context.hpp>

#include <libhoard/cache.h>

SUITE(asio_resolver_policy) {
  class fixture {
    public:
    struct resolver_impl {
      resolver_impl(fixture* self) : self(self) {}

      template<typename CallbackPtr>
      auto operator()(const CallbackPtr& callback_ptr, int n) const -> void {
        ++self->resolver_called_count;
        if (self->error) {
          callback_ptr->assign_error(*self->error);
          self->error = nullptr;
        } else {
          callback_ptr->assign(n, 'x');
        }
      }

      fixture* self;
    };

    using asio_resolver_policy_type = libhoard::asio_resolver_policy<resolver_impl, asio::io_context::executor_type>;
    using cache_type = libhoard::cache<int, std::string, asio_resolver_policy_type>;

    int resolver_called_count = 0;
    asio::io_context io_context;
    std::unique_ptr<int> error; // If set, next callback invocation will install an error.
    cache_type cache = cache_type(asio_resolver_policy_type(resolver_impl(this), this->io_context.get_executor()));
  };

  TEST_FIXTURE(fixture, resolve) {
    int call_count = 0;
    cache.async_get(
        [&call_count](std::string v, int err) {
          ++call_count;
          CHECK_EQUAL("xxx", v);
          CHECK_EQUAL(0, err);
        },
        3);
    io_context.run();

    CHECK_EQUAL(1, resolver_called_count);
    CHECK_EQUAL(1, call_count);
  }

  TEST_FIXTURE(fixture, resolve_error) {
    error = std::make_unique<int>(17);
    int call_count = 0;
    cache.async_get(
        [&call_count](std::string v, int err) {
          ++call_count;
          CHECK_EQUAL("", v);
          CHECK_EQUAL(17, err);
        },
        3);
    io_context.run();

    CHECK_EQUAL(1, resolver_called_count);
    CHECK_EQUAL(1, call_count);
  }

  TEST_FIXTURE(fixture, resolving_the_same_twice_reuses_previous_resolve) {
    int call_count = 0;
    cache.async_get(
        [&call_count](std::string v, int err) {
          ++call_count;
          CHECK_EQUAL("xxx", v);
          CHECK_EQUAL(0, err);
        },
        3);
    cache.async_get(
        [&call_count](std::string v, int err) {
          ++call_count;
          CHECK_EQUAL("xxx", v);
          CHECK_EQUAL(0, err);
        },
        3);
    io_context.run();

    CHECK_EQUAL(1, resolver_called_count);
    CHECK_EQUAL(2, call_count);
  }
}
