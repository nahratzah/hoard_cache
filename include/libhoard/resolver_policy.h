#pragma once

#include <cstddef>
#include <memory>
#include <tuple>
#include <future>
#include <variant>

#include "shared_from_this_policy.h"
#include "detail/meta.h"
#include "detail/refcount.h"
#include "detail/async_resolver_callback.h"
#include "detail/cache_async_get.h"
#include "detail/traits.h"

namespace libhoard {


template<typename Functor>
class resolver_policy {
  public:
  template<typename HashTable, typename ValueType, typename Allocator> class table_base;
  template<typename Impl, typename HashTableType>
  using add_cache_base = detail::cache_base<resolver_policy, Impl, HashTableType>;

  resolver_policy();
  explicit resolver_policy(Functor resolver);

  private:
  Functor resolver_;
};

template<typename Functor>
template<typename HashTable, typename ValueType, typename Allocator>
class resolver_policy<Functor>::table_base {
  public:
  table_base(const resolver_policy& policy, const Allocator& alloc);
  table_base(resolver_policy&& policy, const Allocator& alloc);

  template<typename... Keys>
  auto resolve(std::size_t hash, const Keys&... keys) -> detail::refcount_ptr<ValueType, Allocator>;

  private:
  const Functor resolver_;
};


template<typename Functor>
class async_resolver_policy {
  public:
  using dependencies = detail::type_list<shared_from_this_policy>;

  template<typename HashTable, typename ValueType, typename Allocator> class table_base;
  template<typename Impl, typename HashTableType>
  using add_cache_base = detail::cache_base<async_resolver_policy, Impl, HashTableType>;

  explicit async_resolver_policy(Functor resolver);

  private:
  template<typename HashTable, typename ValuePointer>
  static auto on_assign_(HashTable& hashtable, ValuePointer value_pointer, bool value, bool assigned_via_callback) noexcept -> void;

  const Functor resolver_;
};

template<typename Functor>
template<typename HashTable, typename ValueType, typename Allocator>
class async_resolver_policy<Functor>::table_base {
  public:
  using callback = detail::async_resolver_callback<HashTable, ValueType, Allocator>;

  table_base(const async_resolver_policy& policy, const Allocator& allocator);
  table_base(async_resolver_policy&& policy, const Allocator& allocator);

  template<typename... Keys>
  auto resolve(std::size_t hash, const Keys&... keys) -> detail::refcount_ptr<ValueType, Allocator>;

  const Functor resolver_;
};


} /* namespace libhoard */

namespace libhoard::detail {


template<typename Functor, typename Impl, typename HashTableType>
class cache_base<resolver_policy<Functor>, Impl, HashTableType> {
  protected:
  cache_base() noexcept = default;
  cache_base(const cache_base&) noexcept = default;
  cache_base(cache_base&&) noexcept = default;
  ~cache_base() noexcept = default;
  auto operator=(const cache_base&) noexcept -> cache_base& = default;
  auto operator=(cache_base&&) noexcept -> cache_base& = default;

  public:
  template<typename... Keys>
  auto get(const Keys&... keys) -> typename HashTableType::mapped_type;
};


template<typename Functor, typename Impl, typename HashTableType>
class cache_base<async_resolver_policy<Functor>, Impl, HashTableType> {
  protected:
  cache_base() noexcept = default;
  cache_base(const cache_base&) noexcept = default;
  cache_base(cache_base&&) noexcept = default;
  ~cache_base() noexcept = default;
  auto operator=(const cache_base&) noexcept -> cache_base& = default;
  auto operator=(cache_base&&) noexcept -> cache_base& = default;

  public:
  template<typename... Keys>
  auto get(const Keys&... keys)
  -> std::future<std::variant<typename HashTableType::mapped_type, typename HashTableType::error_type>>;
};


template<typename Functor>
struct has_resolver<resolver_policy<Functor>>
: std::true_type
{};


template<typename Functor>
struct has_async_resolver<async_resolver_policy<Functor>>
: std::true_type
{};


} /* namespace libhoard::detail */

#include "resolver_policy.ii"
