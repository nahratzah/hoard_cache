#pragma once

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <tuple>
#include <utility>

#include "thread_safe_policy.h"
#include "detail/linked_list.h"
#include "detail/meta.h"
#include "detail/refcount.h"

namespace libhoard {


/**
 * \brief Policy that implements refresh logic.
 * \details
 * This is a policy that's used as a dependency by other policies.
 * It installs a `void HashTable::refresh(ValueType*)` function, which refreshes the value.
 * It is safe to call this function multiple times.
 *
 * In order for this policy to work, the cache must have a resolver policy.
 */
class refresh_impl_policy {
  public:
  class value_base;
  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  private:
  template<typename ValueType>
  static auto on_refresh_event(ValueType* new_value, const ValueType* old_value);
};

class refresh_impl_policy::value_base {
  template<typename HashTable, typename ValueType, typename Allocator> friend class refresh_impl_policy::table_base;

  public:
  template<typename HashTable>
  value_base(const HashTable& table);

  private:
  bool refresh_started_ = false;
};

template<typename HashTable, typename ValueType, typename Allocator>
class refresh_impl_policy::table_base {
  public:
  template<typename... Args>
  table_base(const std::tuple<Args...>& args, const Allocator& allocator);

  auto refresh(ValueType* raw_value_ptr) -> void;
};


template<typename Clock>
class refresh_policy {
  private:
  struct tag;

  public:
  using dependencies = detail::type_list<refresh_impl_policy, thread_safe_policy>;
  class value_base;
  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  explicit refresh_policy(typename Clock::duration delay, typename Clock::duration idle_timer = Clock::duration::zero());

  private:
  typename Clock::duration delay, idle_timer;
};

template<typename Clock>
class refresh_policy<Clock>::value_base
: public detail::linked_list_link<tag>
{
  template<typename HashTable, typename ValueType, typename Allocator> friend class refresh_policy::table_base;

  public:
  template<typename HashTable>
  explicit value_base(const HashTable& table);

  auto expired() const noexcept -> bool;
  auto on_refresh(const value_base* old_value) noexcept -> void;

  private:
  typename Clock::time_point refresh_tp;
  typename Clock::time_point cancel_tp = Clock::time_point::max();
};

template<typename Clock>
template<typename HashTable, typename ValueType, typename Allocator>
class refresh_policy<Clock>::table_base {
  public:
  template<typename... Args>
  table_base(const std::tuple<Args...>& args, const Allocator& allocator);

  auto on_assign_(ValueType* vptr, bool value, bool assigned_via_callback) noexcept -> void;
  auto on_hit_(ValueType* vptr) noexcept -> void;
  auto on_unlink_(ValueType* vptr) noexcept -> void;
  auto init() -> void;
  auto destroy() -> void;

  private:
  auto worker_task() -> void;

  typename Clock::duration delay, idle_timer;
  detail::linked_list<ValueType, tag> delay_queue;
  bool stop = false;
  std::thread worker;
  std::condition_variable_any delay_queue_changed;
};


} /* namespace libhoard */

#include "refresh_policy.ii"
