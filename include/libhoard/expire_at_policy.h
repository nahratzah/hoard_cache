#pragma once

namespace libhoard {


/**
 * \brief Policy that marks an object expired at a specific time.
 * \details
 * This policy installs a timestamp in a value.
 *
 * \tparam Clock A type that meets the [chrono Clock requirements](https://en.cppreference.com/w/cpp/named_req/Clock).
 */
template<typename Clock>
class expire_at_policy {
  public:
  class value_base;
};

template<typename Clock>
class expire_at_policy<Clock>::value_base {
  public:
  template<typename Table>
  explicit value_base(const Table& table) noexcept;

  auto expired() const noexcept -> bool;
  auto expire_at(typename Clock::time_point expire_tp) noexcept -> void;

  private:
  typename Clock::time_point expire_tp_ = Clock::time_point::max();
};


} /* namespace libhoard */

#include "expire_at_policy.ii"
