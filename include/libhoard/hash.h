#pragma once

#include <tuple>
#include <type_traits>

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
    private:
    template<typename... Args, typename Alloc>
    table_base_impl(const std::tuple<Args...>& args, [[maybe_unused]] const Alloc& alloc,
        std::true_type has_hash) noexcept
    : hash(std::get<Hash>(args))
    {}

    template<typename... Args, typename Alloc>
    table_base_impl([[maybe_unused]] const std::tuple<Args...>& args, [[maybe_unused]] const Alloc& alloc,
        std::false_type has_hash) noexcept
    : hash()
    {}

    public:
    template<typename... Args, typename Alloc>
    table_base_impl(const std::tuple<Args...>& args, const Alloc& alloc) noexcept
    : table_base_impl(args, alloc, std::disjunction<std::is_same<std::decay_t<Args>, Hash>...>())
    {}

    const Hash hash;
  };

  public:
  template<typename HashTable>
  using table_base = table_base_impl;
};


} /* namespace libhoard */
