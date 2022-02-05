#pragma once

#include "meta.h"

namespace libhoard::detail {


template<typename Tag, typename Cache, typename HashTable> class cache_base;


template<typename Policy, typename Cache, typename HashTable, typename = void>
struct figure_out_cache_base_impl_ {
  using type = void;
};

template<typename Policy, typename Cache, typename HashTable>
struct figure_out_cache_base_impl_<Policy, Cache, HashTable, std::void_t<typename Policy::template add_cache_base<Cache, HashTable>>> {
  using type = typename Policy::template add_cache_base<Cache, HashTable>;
};

template<typename Cache, typename HashTable>
struct figure_out_cache_base_ {
  template<typename Policy>
  using impl = figure_out_cache_base_impl_<Policy, Cache, HashTable>;
};


template<typename... AsyncGetters>
class cache_base_from_policies_impl
: public AsyncGetters...
{
  protected:
  cache_base_from_policies_impl() = default;
  cache_base_from_policies_impl(const cache_base_from_policies_impl&) = default;
  cache_base_from_policies_impl(cache_base_from_policies_impl&&) = default;
  ~cache_base_from_policies_impl() noexcept = default;
  auto operator=(const cache_base_from_policies_impl&) -> cache_base_from_policies_impl& = default;
  auto operator=(cache_base_from_policies_impl&&) noexcept -> cache_base_from_policies_impl& = default;
};

template<typename Cache, typename HashTable>
class cache_base_from_policies
: public HashTable::policy_type_list::template transform_t<figure_out_cache_base_<Cache, HashTable>::template impl>::template remove_all_t<void>::template apply_t<cache_base_from_policies_impl>
{
  protected:
  cache_base_from_policies() = default;
  cache_base_from_policies(const cache_base_from_policies&) = default;
  cache_base_from_policies(cache_base_from_policies&&) = default;
  ~cache_base_from_policies() noexcept = default;
  auto operator=(const cache_base_from_policies&) -> cache_base_from_policies& = default;
  auto operator=(cache_base_from_policies&&) noexcept -> cache_base_from_policies& = default;
};


} /* namespace libhoard::detail */
