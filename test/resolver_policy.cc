#include <libhoard/resolver_policy.h>

#include <exception>
#include <future>
#include <string>
#include <tuple>
#include <thread>
#include <vector>

#include "UnitTest++/UnitTest++.h"

#include <libhoard/detail/hashtable.h>

SUITE(resolver_policy) {
  class fixture {
    public:
    struct resolver_impl {
      auto operator()(int n) const -> std::tuple<std::string::size_type, char> {
        return std::make_tuple(std::string::size_type(n), 'x');
      }
    };

    static_assert(libhoard::detail::has_resolver<libhoard::resolver_policy<resolver_impl>>::value);
    static_assert(libhoard::detail::hashtable_helper_<int, std::string, std::exception_ptr, libhoard::resolver_policy<resolver_impl>>::has_resolver_policy,
        "expected the helper type to detect our resolver_policy");
    static_assert(!libhoard::detail::hashtable_helper_<int, std::string, std::exception_ptr, libhoard::resolver_policy<resolver_impl>>::has_async_resolver_policy,
        "this is not an async resolver policy");

    using hashtable_type = libhoard::detail::hashtable<int, std::string, std::exception_ptr, libhoard::resolver_policy<resolver_impl>>;

    std::unique_ptr<hashtable_type> hashtable = std::make_unique<hashtable_type>(std::make_tuple(libhoard::resolver_policy<resolver_impl>()));
  };

  TEST_FIXTURE(fixture, get) {
    auto three = hashtable->get(3);
    CHECK_EQUAL(1u, three.index());
    CHECK_EQUAL(std::string("xxx"), std::get<1>(three));

    auto four = hashtable->get(4);
    CHECK_EQUAL(1u, four.index());
    CHECK_EQUAL(std::string("xxxx"), std::get<1>(four));
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

    struct thread_resolver_impl {
      thread_resolver_impl(fixture* self) : self(self) {}

      template<typename CallbackPtr>
      auto operator()(CallbackPtr&& callback_ptr, int n) const -> void {
        self->tasks.emplace_back(resolver_impl(self), std::forward<CallbackPtr>(callback_ptr), n);
      }

      fixture*const self;
    };

    using hashtable_type = libhoard::detail::hashtable<int, std::string, std::exception_ptr, libhoard::async_resolver_policy<resolver_impl>>;

    ~fixture() {
      std::for_each(tasks.begin(), tasks.end(), [](std::thread& thr) { thr.join(); });
    }

    std::vector<std::thread> tasks;
    std::exception_ptr error; // If set, next callback invocation will install an error.
    std::shared_ptr<hashtable_type> hashtable = std::make_shared<hashtable_type>(std::make_tuple(libhoard::async_resolver_policy<resolver_impl>(this)));
  };

  class test_exception : public std::exception {};

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
