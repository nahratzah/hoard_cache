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
    table_base_impl(const std::tuple<Args...>& args, const Alloc& alloc) {}

    auto lock() -> void {
      return mtx_.lock();
    }

    auto try_lock() -> bool {
      return mtx_.try_lock();
    }

    auto unlock() -> void {
      return mtx_.unlock();
    }

    private:
    std::mutex mtx_;
  };

  public:
  template<typename HashTable>
  using table_base = table_base_impl;
};


} /* namespace libhoard */
