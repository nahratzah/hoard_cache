#pragma once

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
};

class refresh_impl_policy::value_base {
  template<typename HashTable, typename ValueType, typename Allocator> friend class refresh_impl_policy::table_base;

  public:
  template<typename HashTable>
  value_base([[maybe_unused]] const HashTable& table) {}

  private:
  bool refresh_started_ = false;
};

template<typename HashTable, typename ValueType, typename Allocator>
class refresh_impl_policy::table_base {
  public:
  template<typename... Args>
  table_base([[maybe_unused]] const std::tuple<Args...>& args, [[maybe_unused]] const Allocator& allocator)
  {}

  auto refresh(ValueType* raw_value_ptr) -> void {
    if (raw_value_ptr->refresh_impl_policy::value_base::refresh_started_) return;

    // Acquire owning reference to value.
    auto value_ptr = detail::refcount_ptr<ValueType, Allocator>(static_cast<const HashTable*>(this)->get_allocator());
    value_ptr.reset(raw_value_ptr);

    auto opt_key = value_ptr->key();
    if (!opt_key) {
      // Expire the value so the resolver will perform a refresh the next time this value is looked up.
      value_ptr->mark_expired();
      return;
    }

    auto lookup_ptr = static_cast<HashTable*>(this)->resolve(value_ptr->hash(), std::move(*opt_key));
    value_ptr->refresh_impl_policy::value_base::refresh_started_ = true; // never throws
#if __cpp_exceptions
    try
#endif
    {
      if (typename ValueType::pending_type* pending = lookup_ptr->get_pending()) {
        pending->add_callback(
            [value_ptr]([[maybe_unused]] const auto& value, [[maybe_unused]] const auto& ex) {
              value_ptr->mark_expired();
            });
      } else {
        value_ptr->mark_expired();
      }
    }
#if __cpp_exceptions
    catch (...) {
      // If we failed to install the callback, we mark the old element expired immediately,
      // to ensure it can't linger after the resolve method completes.
      value_ptr->mark_expired();
      throw;
    }
#endif
  }
};


template<typename Clock>
class refresh_policy {
  private:
  struct tag;

  public:
  using dependencies = detail::type_list<refresh_impl_policy, thread_safe_policy>;
  class value_base;
  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  explicit refresh_policy(typename Clock::duration delay, typename Clock::duration idle_timer = Clock::duration::zero())
  : delay(std::move(delay)),
    idle_timer(std::move(idle_timer))
  {}

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
  explicit value_base([[maybe_unused]] const HashTable& table) {}

  auto expired() const noexcept -> bool {
    return Clock::now() >= cancel_tp;
  }

  private:
  typename Clock::time_point refresh_tp;
  typename Clock::time_point cancel_tp = Clock::time_point::max();
};

template<typename Clock>
template<typename HashTable, typename ValueType, typename Allocator>
class refresh_policy<Clock>::table_base {
  public:
  template<typename... Args>
  table_base(const std::tuple<Args...>& args, [[maybe_unused]] const Allocator& allocator)
  : delay(std::get<refresh_policy>(args).delay),
    idle_timer(std::get<refresh_policy>(args).idle_timer)
  {}

  auto on_assign_(ValueType* vptr, bool value, [[maybe_unused]] bool assigned_via_callback) noexcept -> void {
    if (value) {
      vptr->refresh_policy::value_base::refresh_tp = Clock::now() + delay;
      delay_queue.link_back(vptr);
      delay_queue_changed.notify_one();
    }
  }

  auto on_hit_(ValueType* vptr) noexcept -> void {
    if (idle_timer != Clock::duration::zero())
      vptr->cancel_tp = Clock::now() + idle_timer;
  }

  auto on_unlink_(ValueType* vptr) noexcept -> void {
    if (vptr->refresh_policy::value_base::is_linked()) delay_queue.unlink(delay_queue.iterator_to(vptr));
  }

  auto init() -> void {
    worker = std::thread(&table_base::worker_task, this);
  }

  auto destroy() -> void {
    {
      auto lck = std::lock_guard<HashTable>(*static_cast<HashTable*>(this));
      stop = true;
      delay_queue_changed.notify_all();
    }

    if (worker.joinable()) worker.join();
  }

  private:
  void worker_task() {
    auto self = static_cast<HashTable*>(this);
    auto lck = std::unique_lock<HashTable>(*self);

    if (stop) return;
    for (;;) {
      if (delay_queue.empty())
        delay_queue_changed.wait(lck);
      else
        delay_queue_changed.wait_until(lck, delay_queue.begin()->refresh_tp);
      if (stop) return;

      const auto now = Clock::now();
      while (!delay_queue.empty() && now >= delay_queue.begin()->refresh_tp) {
        ValueType* vptr = delay_queue.begin().get();
        delay_queue.unlink(delay_queue.iterator_to(vptr));
        if (!vptr->expired())
          self->refresh(vptr); // XXX should we swallow exceptions?
      }
    }
  }

  typename Clock::duration delay, idle_timer;
  detail::linked_list<ValueType, tag> delay_queue;
  bool stop = false;
  std::thread worker;
  std::condition_variable_any delay_queue_changed;
};


} /* namespace libhoard */
