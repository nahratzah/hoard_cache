#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include <asio/associated_allocator.hpp>
#include <asio/associated_executor.hpp>
#include <asio/async_result.hpp>
#include <asio/executor_work_guard.hpp>

#include "../detail/async_resolver_callback.h"
#include "../detail/cache_async_get.h"
#include "../detail/meta.h"
#include "../detail/refcount.h"
#include "../detail/traits.h"
#include "../shared_from_this_policy.h"

namespace libhoard {


template<typename Functor, typename Executor = typename ::asio::associated_executor<Functor>::type>
class asio_resolver_policy {
  public:
  using dependencies = detail::type_list<shared_from_this_policy>;

  template<typename HashTable, typename ValueType, typename Allocator> class table_base;
  template<typename Impl, typename HashTableType>
  using async_getter = detail::async_getter_impl<asio_resolver_policy, Impl, HashTableType>;

  asio_resolver_policy(Functor functor, Executor executor)
  : executor(std::move(executor)),
    functor(std::move(functor))
  {}

  explicit asio_resolver_policy(Functor functor)
  : executor(::asio::associated_executor<Functor>::get(functor)),
    functor(std::move(functor))
  {}

  private:
  Executor executor;
  Functor functor;
};

template<typename Functor, typename Executor>
template<typename HashTable, typename ValueType, typename Allocator>
class asio_resolver_policy<Functor, Executor>::table_base {
  private:
  template<typename... Keys> class wrapper;

  public:
  using callback = detail::async_resolver_callback<HashTable, ValueType, Allocator>;

  template<typename... Args>
  table_base(const std::tuple<Args...>& args, [[maybe_unused]] const Allocator& alloc)
  : executor(std::get<asio_resolver_policy>(args).executor),
    functor(std::get<asio_resolver_policy>(args).functor)
  {}

  auto get_executor() const -> Executor {
    return executor;
  }

  template<typename... Keys>
  auto resolve(std::size_t hash, const Keys&... keys) -> detail::refcount_ptr<ValueType, Allocator> {
    using callback_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<callback>;

    auto self = static_cast<HashTable*>(this);
    auto new_value = self->allocate_value_type(std::piecewise_construct, std::forward_as_tuple(keys...));

    self->link(hash, new_value);
    executor.dispatch(
        [functor=this->functor, cb=std::allocate_shared<callback>(callback_allocator_type(self->get_allocator()), self->shared_from_this(), new_value), keys...]() mutable {
          std::invoke(std::move(functor), std::move(cb), std::move(keys)...);
        },
        self->get_allocator());
    return new_value;
  }

  private:
  Executor executor;
  Functor functor;
};


} /* namespace libhoard */

namespace libhoard::detail {


template<typename Functor, typename Executor, typename Impl, typename HashTableType>
class async_getter_impl<asio_resolver_policy<Functor, Executor>, Impl, HashTableType> {
  public:
  using executor_type = Executor;

  protected:
  async_getter_impl() noexcept = default;
  async_getter_impl(const async_getter_impl&) noexcept = default;
  async_getter_impl(async_getter_impl&&) noexcept = default;
  ~async_getter_impl() noexcept = default;
  auto operator=(const async_getter_impl&) noexcept -> async_getter_impl& = default;
  auto operator=(async_getter_impl&&) noexcept -> async_getter_impl& = default;

  public:
  auto get_executor() const -> executor_type {
    const Impl*const self = static_cast<const Impl*>(this);
    return self->impl_->get_executor();
  }

  template<typename CompletionToken, typename... Keys>
  auto async_get(CompletionToken&& token, const Keys&... keys)
  -> decltype(std::declval<::asio::async_result<std::decay_t<CompletionToken>, void(typename HashTableType::mapped_type, typename HashTableType::error_type)>>().get()) {
    Impl*const self = static_cast<Impl*>(this);
    std::lock_guard<HashTableType> lck{ *self->impl_ };

    using async_result = ::asio::async_result<std::decay_t<CompletionToken>, void(typename HashTableType::mapped_type, typename HashTableType::error_type)>;
    using completion_handler_type = typename async_result::completion_handler_type;
    completion_handler_type completion_handler(std::forward<CompletionToken>(token));
    async_result result(completion_handler);
    ::asio::associated_executor_t<typename async_result::completion_handler_type, Executor> executor = ::asio::get_associated_executor(completion_handler, self->impl_->get_executor());

    self->impl_->async_get(
        [ work=::asio::executor_work_guard<::asio::associated_executor_t<completion_handler_type, Executor>>(executor),
          completion_handler=std::move(completion_handler),
          alloc1=self->impl_->get_allocator()
        ](typename HashTableType::mapped_type value, typename HashTableType::error_type err, auto is_immediately_resolved) mutable {
          auto alloc = ::asio::get_associated_allocator(completion_handler, std::move(alloc1));
          auto callback = [ completion_handler=std::move(completion_handler),
                            value=std::move(value),
                            err=std::move(err)
                          ]() mutable {
            std::invoke(std::move(completion_handler), std::move(value), std::move(err));
          };

          if constexpr(is_immediately_resolved())
            work.get_executor().post(std::move(callback), std::move(alloc));
          else
            work.get_executor().dispatch(std::move(callback), std::move(alloc));
        },
        keys...);

    return result.get();
  }
};


template<typename Functor, typename Executor>
struct has_async_resolver<asio_resolver_policy<Functor, Executor>>
: public std::true_type
{};


} /* namespace libhoard */
