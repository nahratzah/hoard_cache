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
        [&call_count, &delay, this](std::string v, int err) {
          ++call_count;
          CHECK_EQUAL("xxx", v);
          CHECK_EQUAL(0, err);

          delay.expires_after(1s + 500ms);
          delay.async_wait(
              [&call_count, this](const auto& err) {
                if (err) return;
                this->cache.async_get(
                    [&call_count](std::string v, int err) {
                      ++call_count;
                      CHECK_EQUAL("yyy", v);
                      CHECK_EQUAL(0, err);
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
