#pragma once

#include <cstddef>
#include <tuple>

#include "detail/meta.h"
#include "detail/queue.h"

namespace libhoard {


/**
 * \brief Policy that restricts the number of elements in the cache.
 */
class max_size_policy {
  public:
  using dependencies = detail::type_list<detail::queue_policy>;

  ///\brief Policy implementation.
  ///\tparam HashTable The derived hashtable type.
  ///\tparam ValueType Type held by the hashtable. (This parameter is unused.)
  template<typename HashTable, typename ValueType, typename Allocator>
  class table_base {
    public:
    ///\brief Constructor.
    ///\note Requires one of the elements in \p args to be max_size
    template<typename... Args>
    table_base(const std::tuple<Args...>& args, const Allocator& alloc) noexcept;

    ///\brief Tell the cache to maybe expire some elements.
    auto policy_removal_check_() const noexcept -> std::size_t;

    private:
    std::size_t max_size_;
  };

  explicit max_size_policy(std::size_t v) noexcept;

  private:
  std::size_t value;
};


} /* namespace libhoard */

#include "max_size_policy.ii"
