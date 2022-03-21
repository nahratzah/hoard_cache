#pragma once

#include <condition_variable>
#include <thread>
#include <tuple>

#include "thread_safe_policy.h"
#include "detail/linked_list.h"
#include "detail/meta.h"
#include "detail/refresh_impl_policy.h"

namespace libhoard {


template<typename Clock>
class refresh_policy {
  private:
  struct tag;

  public:
  using dependencies = detail::type_list<detail::refresh_impl_policy, thread_safe_policy>;
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
  table_base(const refresh_policy& policy, const Allocator& allocator);
  table_base(refresh_policy&& policy, const Allocator& allocator);

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
