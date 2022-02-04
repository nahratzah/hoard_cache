#pragma once

#include <tuple>

#include <asio/system_executor.hpp>
#include <asio/wait_traits.hpp>
#include <asio/basic_waitable_timer.hpp>

#include "../detail/meta.h"
#include "../detail/refresh_impl_policy.h"

namespace libhoard {


template<typename Clock, typename Executor = asio::system_executor, typename WaitTraits = asio::wait_traits<Clock>>
class asio_refresh_policy {
  public:
  using dependencies = detail::type_list<detail::refresh_impl_policy>;
  class value_base;
  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  explicit asio_refresh_policy(Executor executor, typename Clock::duration delay, typename Clock::duration idle_timer = Clock::duration::zero());

  private:
  typename Clock::duration delay, idle_timer;
  Executor executor;
};

template<typename Clock, typename Executor, typename WaitTraits>
class asio_refresh_policy<Clock, Executor, WaitTraits>::value_base {
  template<typename HashTable, typename ValueType, typename Allocator> friend class asio_refresh_policy::table_base;

  public:
  template<typename HashTable, typename ValueType, typename Allocator>
  explicit value_base(const table_base<HashTable, ValueType, Allocator>& table);

  auto expired() const noexcept -> bool;
  auto on_refresh(const value_base* old_value) noexcept -> void;

  private:
  asio::basic_waitable_timer<Clock, WaitTraits, Executor> refresh;
  typename Clock::time_point cancel_tp = Clock::time_point::max();
};

template<typename Clock, typename Executor, typename WaitTraits>
template<typename HashTable, typename ValueType, typename Allocator>
class asio_refresh_policy<Clock, Executor, WaitTraits>::table_base {
  friend asio_refresh_policy::value_base;

  public:
  template<typename... Args>
  table_base(const std::tuple<Args...>& args, [[maybe_unused]] const Allocator& allocator);

  auto on_assign_(ValueType* vptr, bool value, bool assigned_via_callback) noexcept -> void;
  auto on_hit_(ValueType* vptr) noexcept -> void;
  auto on_unlink_(value_base* vptr) noexcept -> void;

  private:
  typename Clock::duration delay, idle_timer;
  Executor executor;
};


} /* namespace libhoard */

#include "refresh_policy.ii"
