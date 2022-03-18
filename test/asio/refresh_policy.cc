#include <libhoard/asio/refresh_policy.h>

#include <chrono>
#include <exception>
#include <list>
#include <string>

#include "UnitTest++/UnitTest++.h"

#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>

#include <libhoard/asio/resolver_policy.h>
#include <libhoard/cache.h>

SUITE(asio_refresh_policy) {
  class fixture {
    public:
    struct resolver_impl {
      resolver_impl(fixture* self) : self(self) {}

      template<typename CallbackPtr>
      auto operator()(const CallbackPtr& callback_ptr, int n) const -> void {
        CHECK_EQUAL(3, n);
        ++self->resolver_called_count;

        auto result = self->resolver_values.front();
        if (self->resolver_values.size() > 1)
          self->resolver_values.pop_front();
        callback_ptr->assign(result);
      }

      fixture* self;
    };

    using asio_resolver_policy_type = libhoard::asio_resolver_policy<resolver_impl, asio::io_context::executor_type>;
    using asio_refresh_policy_type = libhoard::asio_refresh_policy<std::chrono::system_clock, asio::io_context::executor_type>;
    using cache_type = libhoard::cache<int, std::string, asio_resolver_policy_type, asio_refresh_policy_type>;

    int resolver_called_count = 0;
    std::list<std::string> resolver_values;
    asio::io_context io_context;
    cache_type cache = cache_type(asio_resolver_policy_type(resolver_impl(this), this->io_context.get_executor()), asio_refresh_policy_type(this->io_context.get_executor(), std::chrono::seconds(1), std::chrono::seconds(3)));
  };

  TEST_FIXTURE(fixture, resolve) {
    using namespace std::literals::chrono_literals;

    int call_count = 0;
    resolver_values = { "xxx", "yyy" };
    typename asio::steady_timer::rebind_executor<asio::io_context::executor_type>::other delay(io_context.get_executor());

    cache.async_get(
        [&call_count, &delay, this](std::string v, std::exception_ptr err) {
          ++call_count;
          CHECK_EQUAL("xxx", v);
          CHECK(err == nullptr);

          delay.expires_after(1s + 500ms);
          delay.async_wait(
              [&call_count, this](const auto& err) {
                if (err) return;
                this->cache.async_get(
                    [&call_count](std::string v, std::exception_ptr err) {
                      ++call_count;
                      CHECK_EQUAL("yyy", v);
                      CHECK(err == nullptr);
                    },
                    3);
              });
        },
        3);

    io_context.run();

    CHECK(resolver_called_count >= 2);
    CHECK_EQUAL(2, call_count);
  }
}


SUITE(asio_refresh_fn_policy) {
  class fixture {
    public:
    struct resolver_impl {
      resolver_impl(fixture* self) : self(self) {}

      template<typename CallbackPtr>
      auto operator()(const CallbackPtr& callback_ptr, int n) const -> void {
        CHECK_EQUAL(3, n);
        ++self->resolver_called_count;

        auto result = self->resolver_values.front();
        if (self->resolver_values.size() > 1)
          self->resolver_values.pop_front();
        callback_ptr->assign(result);
      }

      fixture* self;
    };

    struct refresh_tp_impl {
      refresh_tp_impl(fixture* self) : self(self) {}

      auto operator()([[maybe_unused]] const std::string& s) const -> std::chrono::system_clock::time_point {
        ++self->refresh_tp_called_count;

        auto result = self->refresh_tps.front();
        if (self->refresh_tps.size() > 1)
          self->refresh_tps.pop_front();
        return result;
      }

      fixture* self;
    };

    using asio_resolver_policy_type = libhoard::asio_resolver_policy<resolver_impl, asio::io_context::executor_type>;
    using asio_refresh_policy_type = libhoard::asio_refresh_fn_policy<std::chrono::system_clock, refresh_tp_impl, asio::io_context::executor_type>;
    using cache_type = libhoard::cache<int, std::string, asio_resolver_policy_type, asio_refresh_policy_type>;

    int resolver_called_count = 0, refresh_tp_called_count = 0;
    std::list<std::string> resolver_values;
    std::list<std::chrono::system_clock::time_point> refresh_tps;
    asio::io_context io_context;
    cache_type cache = cache_type(asio_resolver_policy_type(resolver_impl(this), this->io_context.get_executor()), asio_refresh_policy_type(this->io_context.get_executor(), refresh_tp_impl(this), std::chrono::seconds(2)));
  };

  TEST_FIXTURE(fixture, resolve) {
    using namespace std::literals::chrono_literals;

    int call_count = 0;
    resolver_values = { "xxx", "yyy" };
    refresh_tps = { std::chrono::system_clock::now() + 1s, std::chrono::system_clock::now() + 5s };
    typename asio::steady_timer::rebind_executor<asio::io_context::executor_type>::other delay(io_context.get_executor());

    cache.async_get(
        [&call_count, &delay, this](std::string v, std::exception_ptr err) {
          ++call_count;
          CHECK_EQUAL("xxx", v);
          CHECK(err == nullptr);

          delay.expires_after(1s + 500ms);
          delay.async_wait(
              [&call_count, this](const auto& err) {
                if (err) return;
                this->cache.async_get(
                    [&call_count](std::string v, std::exception_ptr err) {
                      ++call_count;
                      CHECK_EQUAL("yyy", v);
                      CHECK(err == nullptr);
                    },
                    3);
              });
        },
        3);

    io_context.run();

    CHECK_EQUAL(2, resolver_called_count);
    CHECK_EQUAL(2, refresh_tp_called_count);
    CHECK_EQUAL(2, call_count);
  }
}
