#pragma once

#include <type_traits>
#include <chrono>

#include "expire_at_policy.h"
#include "negative_cache_policy.h"
#include "detail/meta.h"

namespace libhoard {


template<typename Clock = std::chrono::system_clock, bool ExpectValue = true>
class max_age_policy_impl {
  private:
  using clock_dependencies = std::conditional_t<
      Clock::is_steady,
      detail::type_list<expire_at_policy<Clock>>,
      detail::type_list<expire_at_policy<Clock>, expire_at_policy<std::chrono::steady_clock>>>;
  using expect_dependencies = std::conditional_t<
      ExpectValue,
      detail::type_list<>,
      detail::type_list<negative_cache_policy>>;

  public:
  using dependencies = typename clock_dependencies::template extend_t<expect_dependencies>;

  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  max_age_policy_impl() = delete;
  explicit max_age_policy_impl(std::chrono::milliseconds max_age);

  private:
  std::chrono::milliseconds max_age;
};

template<typename Clock, bool ExpectValue>
template<typename HashTable, typename ValueType, typename Allocator>
class max_age_policy_impl<Clock, ExpectValue>::table_base {
  public:
  template<typename... Args>
  table_base(const std::tuple<Args...>& args, const Allocator& alloc) noexcept;

  auto on_assign_(ValueType* vptr, bool value, [[maybe_unused]] bool assigned_via_callback) noexcept -> void;

  private:
  std::chrono::milliseconds max_age_;
};


/**
 * \brief Control how long values are cached.
 *
 * \details
 * This policy installs a duration using the expire_at_policy.
 *
 * The policy ensures against the clock being changed, by installing
 * the duration on the monotonic clock too, if \p Clock isn't monotonic.
 *
 * \tparam Clock The std::chrono clock used to measure duration.
 */
template<typename Clock = std::chrono::system_clock>
using max_age_policy = max_age_policy_impl<Clock, true>;

/**
 * \brief Control how long errors are cached.
 *
 * \details
 * This policy installs a duration using the expire_at_policy.
 *
 * The policy ensures against the clock being changed, by installing
 * the duration on the monotonic clock too, if \p Clock isn't monotonic.
 *
 * \tparam Clock The std::chrono clock used to measure duration.
 */
template<typename Clock = std::chrono::system_clock>
using error_max_age_policy = max_age_policy_impl<Clock, false>;


} /* namespace libhoard */

#include "max_age_policy.ii"
