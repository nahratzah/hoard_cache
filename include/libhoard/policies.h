#pragma once

#include <cstddef>
#include <tuple>

#include "detail/meta.h"
#include "equal.h"
#include "hash.h"

namespace libhoard::detail {
struct queue_policy; // Forward declaration, implemented in "detail/queue.h"
} /* namespace libhoard::detail */

namespace libhoard {


using detail::type_list;
using detail::queue_policy;


/**
 * \brief Policy that makes the cache weaken elements.
 * \details
 * If this policy is added to the cache, elements will be weakened.
 * This means that instead of removing the item from the cache, its
 * pointer will be turned into a weak pointer.
 *
 * This policy has no effect if the cache doesn't hold smart pointers.
 *
 * \note This policy doesn't add base types, instead the queue code checks
 * for presence and modifies behaviour accordingly.
 */
class weaken_policy {};


/**
 * \brief Policy that restricts the number of elements in the cache.
 */
class max_size_policy {
  public:
  using dependencies = type_list<queue_policy>;

  ///\brief Value that is to be passed into the cache, to set the max size.
  class max_size {
    public:
    max_size(std::size_t v) noexcept;

    std::size_t value;
  };

  ///\brief Policy implementation.
  ///\tparam HashTable The derived hashtable type.
  template<typename HashTable>
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

#include "policies.ii"
#include "detail/queue.h" // Late inclusion, because it depends on types in this file.
