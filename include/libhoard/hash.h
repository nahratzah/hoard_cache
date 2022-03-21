#pragma once

#include <utility>

namespace libhoard {


/**
 * \brief Policy that controls the hash function for the hashtable.
 * \tparam Hash Hash functor.
 * \ingroup libhoard_api
 */
template<typename Hash>
class hash {
  private:
  class table_base_impl {
    public:
    template<typename Alloc>
    table_base_impl(const class hash& policy, [[maybe_unused]] const Alloc& alloc)
    : hash(policy.hash_)
    {}

    template<typename Alloc>
    table_base_impl(class hash&& policy, [[maybe_unused]] const Alloc& alloc)
    : hash(std::move(policy.hash_))
    {}

    const Hash hash;
  };

  public:
  template<typename HashTable, typename ValueType, typename Allocator>
  using table_base = table_base_impl;

  hash() = default;

  explicit hash(const Hash& hash_fn)
  : hash_(hash_fn)
  {}

  explicit hash(Hash&& hash_fn)
  : hash_(std::move(hash_fn))
  {}

  private:
  Hash hash_;
};


} /* namespace libhoard */
