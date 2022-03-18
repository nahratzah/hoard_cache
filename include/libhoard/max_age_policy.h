#pragma once

#include <type_traits>
#include <chrono>

#include "expire_at_policy.h"
#include "detail/meta.h"

namespace libhoard {


template<typename Clock = std::chrono::system_clock>
class max_age_policy {
  public:
  using dependencies = std::conditional_t<
      Clock::is_steady,
      detail::type_list<expire_at_policy<Clock>>,
      detail::type_list<expire_at_policy<Clock>, expire_at_policy<std::chrono::steady_clock>>>;

  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  max_age_policy() = delete;
  explicit max_age_policy(std::chrono::milliseconds max_age);

  private:
  std::chrono::milliseconds max_age;
};

template<typename Clock>
template<typename HashTable, typename ValueType, typename Allocator>
class max_age_policy<Clock>::table_base {
  public:
  template<typename... Args>
  table_base(const std::tuple<Args...>& args, const Allocator& alloc) noexcept;

  auto on_assign_(ValueType* vptr, bool value, [[maybe_unused]] bool assigned_via_callback) noexcept -> void;

  private:
  std::chrono::milliseconds max_age_;
};


} /* namespace libhoard */

#include "max_age_policy.ii"
