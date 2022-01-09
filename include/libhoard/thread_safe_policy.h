#pragma once

#include <mutex>
#include <tuple>

namespace libhoard {


/**
 * \brief Policy that protects the hashtable from concurrent access.
 * \details
 * Makes the thread lockable and implements this by forwarding lock/unlock calls
 * to a mutex.
 */
class thread_safe_policy {
  private:
  ///\brief Policy implementation.
  ///\tparam HashTable The derived hashtable type.
  class table_base_impl {
    public:
    ///\brief Constructor.
    template<typename... Args, typename Alloc>
    table_base_impl(const std::tuple<Args...>& args, const Alloc& alloc);

    auto lock() -> void;
    auto try_lock() -> bool;
    auto unlock() -> void;

    private:
    std::recursive_mutex mtx_;
  };

  public:
  template<typename HashTable, typename ValueType, typename Allocator>
  using table_base = table_base_impl;
};


} /* namespace libhoard */

#include "thread_safe_policy.ii"
