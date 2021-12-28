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

  ///\brief Value that is to be passed into the cache, to set the max size.
  class max_size {
    public:
    max_size(std::size_t v) noexcept;

    std::size_t value;
  };

  ///\brief Policy implementation.
  ///\tparam HashTable The derived hashtable type.
  ///\tparam ValueType Type held by the hashtable. (This parameter is unused.)
  template<typename HashTable, typename ValueType, typename Allocator>
  class table_base {
    public:
    ///\brief Constructor.
    ///\note Requires one of the elements in \p args to be max_size
    template<typename... Args, typename Alloc>
    table_base(const std::tuple<Args...>& args, const Alloc& alloc) noexcept;

    ///\brief Tell the cache to maybe expire some elements.
    auto policy_removal_check_() const noexcept -> std::size_t;

    private:
    std::size_t max_size_;
  };
};


} /* namespace libhoard */

#include "max_size_policy.ii"
