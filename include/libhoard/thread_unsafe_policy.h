#pragma once

#include <tuple>

namespace libhoard {


/**
 * \brief Policy that adds lock and unlock noop methods.
 * \details
 * The idea is that the hashtable is still lockable.
 * But that those locks don't function.
 * This removes the need for conditionally including mutex lock/unlock code.
 */
class thread_unsafe_policy {
  private:
  class table_base_impl {
    public:
    template<typename... Args, typename Alloc>
    table_base_impl([[maybe_unused]] const std::tuple<Args...>& args, [[maybe_unused]] const Alloc& alloc) {}

    auto lock() const noexcept -> void {}

    constexpr auto try_lock() const noexcept -> bool {
      return true;
    }

    auto unlock() const noexcept -> void {}
  };

  public:
  template<typename HashTable, typename ValueType, typename Allocator>
  using table_base = table_base_impl;
};


} /* namespace libhoard::detail */
