#pragma once

#include <utility>

namespace libhoard {


/**
 * \brief Policy that controls the equality function for the hashtable.
 * \tparam Equal Equality operator that is used by the hashtable.
 * \ingroup libhoard_api
 */
template<typename Equal>
class equal {
  private:
  class table_base_impl {
    public:
    template<typename Alloc>
    table_base_impl(const class equal& policy, [[maybe_unused]] const Alloc& alloc)
    : equal(policy.equal_)
    {}

    template<typename Alloc>
    table_base_impl(class equal&& policy, [[maybe_unused]] const Alloc& alloc)
    : equal(std::move(policy.equal_))
    {}

    const Equal equal;
  };

  public:
  template<typename HashTable, typename ValueType, typename Allocator>
  using table_base = table_base_impl;

  equal() = default;

  explicit equal(const Equal& equal_fn)
  : equal_(equal_fn)
  {}

  explicit equal(Equal&& equal_fn)
  : equal_(std::move(equal_fn))
  {}

  private:
  Equal equal_;
};


} /* namespace libhoard */
