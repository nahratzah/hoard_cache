#pragma once

#include "meta.h"

namespace libhoard::detail {


template<typename Tag, typename Cache, typename HashTable> class async_getter_impl;


template<typename Policy, typename Cache, typename HashTable, typename = void>
struct figure_out_async_getter_impl_ {
  using type = void;
};

template<typename Policy, typename Cache, typename HashTable>
struct figure_out_async_getter_impl_<Policy, Cache, HashTable, std::void_t<typename Policy::template async_getter<Cache, HashTable>>> {
  using type = typename Policy::template async_getter<Cache, HashTable>;
};

template<typename Cache, typename HashTable>
struct figure_out_async_getter_ {
  template<typename Policy>
  using impl = figure_out_async_getter_impl_<Policy, Cache, HashTable>;
};


template<typename... AsyncGetters>
class async_getter_from_policies_impl
: public AsyncGetters...
{
  protected:
  async_getter_from_policies_impl() = default;
  async_getter_from_policies_impl(const async_getter_from_policies_impl&) = default;
  async_getter_from_policies_impl(async_getter_from_policies_impl&&) = default;
  ~async_getter_from_policies_impl() noexcept = default;
  auto operator=(const async_getter_from_policies_impl&) -> async_getter_from_policies_impl& = default;
  auto operator=(async_getter_from_policies_impl&&) noexcept -> async_getter_from_policies_impl& = default;

  public:
  using AsyncGetters::async_get...;
};

template<typename Cache, typename HashTable>
class async_getter_from_policies
: public HashTable::policy_type_list::template transform_t<figure_out_async_getter_<Cache, HashTable>::template impl>::template remove_all_t<void>::template apply_t<async_getter_from_policies_impl>
{
  protected:
  async_getter_from_policies() = default;
  async_getter_from_policies(const async_getter_from_policies&) = default;
  async_getter_from_policies(async_getter_from_policies&&) = default;
  ~async_getter_from_policies() noexcept = default;
  auto operator=(const async_getter_from_policies&) -> async_getter_from_policies& = default;
  auto operator=(async_getter_from_policies&&) noexcept -> async_getter_from_policies& = default;
};


} /* namespace libhoard::detail */
