#pragma once

#include <tuple>
#include <type_traits>

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
    private:
    template<typename... Args, typename Alloc>
    table_base_impl(const std::tuple<Args...>& args, [[maybe_unused]] const Alloc& alloc,
        std::true_type has_equal) noexcept
    : equal(std::get<Equal>(args))
    {}

    template<typename... Args, typename Alloc>
    table_base_impl([[maybe_unused]] const std::tuple<Args...>& args, [[maybe_unused]] const Alloc& alloc,
        std::false_type has_equal) noexcept
    : equal()
    {}

    public:
    template<typename... Args, typename Alloc>
    table_base_impl(const std::tuple<Args...>& args, const Alloc& alloc) noexcept
    : table_base_impl(args, alloc, std::disjunction<std::is_same<std::decay_t<Args>, Equal>...>())
    {}

    const Equal equal;
  };

  public:
  template<typename HashTable>
  using table_base = table_base_impl;
};


} /* namespace libhoard */
