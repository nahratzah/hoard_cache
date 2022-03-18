#include <libhoard/resolver_policy.h>

#include <exception>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/hashtable.h>
#include <libhoard/cache.h>

SUITE(resolver_policy) {
  class fixture {
    public:
    struct resolver_impl {
      explicit resolver_impl(fixture& self) noexcept
      : self(&self)
      {}

      auto operator()(int n) const -> std::tuple<std::string::size_type, char> {
        if (self->error) std::rethrow_exception(std::exchange(self->error, nullptr));
        return std::make_tuple(std::string::size_type(n), 'x');
      }

      private:
      fixture* self;
    };

    static_assert(libhoard::detail::has_resolver<libhoard::resolver_policy<resolver_impl>>::value);
    static_assert(libhoard::detail::hashtable_helper_<int, std::string, std::exception_ptr, libhoard::resolver_policy<resolver_impl>>::has_resolver_policy,
        "expected the helper type to detect our resolver_policy");
    static_assert(!libhoard::detail::hashtable_helper_<int, std::string, std::exception_ptr, libhoard::resolver_policy<resolver_impl>>::has_async_resolver_policy,
        "this is not an async resolver policy");

    using hashtable_type = libhoard::detail::hashtable<int, std::string, std::exception_ptr, libhoard::resolver_policy<resolver_impl>>;
    using cache_type = libhoard::cache<int, std::string, libhoard::resolver_policy<resolver_impl>>;

    std::unique_ptr<hashtable_type> hashtable = std::make_unique<hashtable_type>(std::make_tuple(libhoard::resolver_policy<resolver_impl>(resolver_impl(*this))));
    cache_type cache = cache_type(libhoard::resolver_policy<resolver_impl>(resolver_impl(*this)));
    std::exception_ptr error;
  };

  TEST_FIXTURE(fixture, get) {
    auto three = cache.get(3);
    CHECK_EQUAL(std::string("xxx"), three);

    auto four = cache.get(4);
    CHECK_EQUAL(std::string("xxxx"), four);
  }

  TEST_FIXTURE(fixture, async_get) {
    std::future<std::string> three;

    {
      std::promise<std::string> three_prom;
      three = three_prom.get_future();
      hashtable->async_get(
          [three_prom=std::move(three_prom)](const std::string& value, const std::exception_ptr& error, auto immediate) mutable {
            CHECK(immediate() == true);

            if (error)
              three_prom.set_exception(error);
            else
              three_prom.set_value(value);
          },
          3);
    }

    CHECK_EQUAL(std::string("xxx"), three.get());
  }

  TEST_FIXTURE(fixture, get_does_not_cache_errors) {
    const auto error_1 = std::make_exception_ptr(std::runtime_error("'error one'"));
    const auto error_2 = std::make_exception_ptr(std::runtime_error("'error two'"));
    bool error_one_seen = false, error_two_seen = false;

    error = error_1;
    try {
      cache.get(7);
    } catch (const std::exception& ex) {
      CHECK_EQUAL("'error one'", ex.what());
      error_one_seen = true;
    }

    error = error_2;
    try {
      cache.get(7);
    } catch (const std::exception& ex) {
      CHECK_EQUAL("'error two'", ex.what());
      error_two_seen = true;
    }

    CHECK(error_one_seen);
    CHECK(error_two_seen);
  }
}

SUITE(async_resolver_policy) {
  class fixture {
    public:
    struct resolver_impl {
      resolver_impl(fixture* self) : self(self) {}

      template<typename CallbackPtr>
      auto operator()(const CallbackPtr& callback_ptr, int n) const -> void {
        if (self->error) {
          callback_ptr->assign_error(self->error);
          self->error = nullptr;
        } else {
          callback_ptr->assign(n, 'x');
        }
      }

      fixture*const self;
    };

    struct resolver_impl_for_int_errors {
      resolver_impl_for_int_errors(fixture* self) : self(self) {}

      template<typename CallbackPtr>
      auto operator()(const CallbackPtr& callback_ptr, int n) const -> void {
        if (self->error) {
          callback_ptr->assign_error(self->error);
          self->error = nullptr;
        } else {
          callback_ptr->assign(n, 'x');
        }
      }

      fixture*const self;
    };

    struct thread_resolver_impl {
      thread_resolver_impl(fixture* self) : self(self) {}

      template<typename CallbackPtr>
      auto operator()(CallbackPtr&& callback_ptr, int n) const -> void {
        self->tasks.emplace_back(resolver_impl(self), std::forward<CallbackPtr>(callback_ptr), n);
      }

      fixture*const self;
    };

    using hashtable_type = libhoard::detail::hashtable<int, std::string, std::exception_ptr, libhoard::async_resolver_policy<resolver_impl>>;
    using cache_type = libhoard::cache<int, std::string, libhoard::async_resolver_policy<resolver_impl_for_int_errors>>;

    ~fixture() {
      std::for_each(tasks.begin(), tasks.end(), [](std::thread& thr) { thr.join(); });
    }

    std::vector<std::thread> tasks;
    std::exception_ptr error; // If set, next callback invocation will install an error.
    std::shared_ptr<hashtable_type> hashtable = std::make_shared<hashtable_type>(std::make_tuple(libhoard::async_resolver_policy<resolver_impl>(this)));
    cache_type cache = cache_type(libhoard::async_resolver_policy<resolver_impl_for_int_errors>(this));
  };

  class test_exception : public std::exception {};

  TEST_FIXTURE(fixture, async_get) {
    auto variant_future = cache.get(3);
    CHECK_EQUAL(std::string("xxx"), std::get<0>(variant_future.get()));
  }

  TEST_FIXTURE(fixture, async_get_with_error) {
    const std::exception_ptr expected_error = std::make_exception_ptr(test_exception());
    this->error = expected_error; // Tell fixute to emit an error.
    std::future<std::string> three;

    {
      std::promise<std::string> three_prom;
      three = three_prom.get_future();
      hashtable->async_get(
          [three_prom=std::move(three_prom)](const std::string& value, const std::exception_ptr& error, auto immediate) mutable {
            CHECK(immediate() == true);

            if (error)
              three_prom.set_exception(error);
            else
              three_prom.set_value(value);
          },
          3);
    }

    CHECK_THROW(three.get(), test_exception);
  }
}
