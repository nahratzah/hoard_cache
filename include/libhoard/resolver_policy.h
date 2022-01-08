#pragma once

#include <cstddef>
#include <memory>
#include <tuple>

#include "shared_from_this_policy.h"
#include "detail/meta.h"
#include "detail/refcount.h"

namespace libhoard {


template<typename Functor>
class resolver_policy {
  public:
  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  resolver_policy();
  explicit resolver_policy(Functor resolver);

  private:
  Functor resolver_;
};

template<typename Functor>
template<typename HashTable, typename ValueType, typename Allocator>
class resolver_policy<Functor>::table_base {
  public:
  template<typename... Args>
  table_base(const std::tuple<Args...>& args, const Allocator& alloc);

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
  class callback;

  template<typename... Args>
  table_base(const std::tuple<Args...>& args, const Allocator& alloc);

  template<typename... Keys>
  auto async_resolve(std::size_t hash, const Keys&... keys) -> detail::refcount_ptr<ValueType, Allocator>;

  const Functor resolver_;
};

template<typename Functor>
template<typename HashTable, typename ValueType, typename Allocator>
class async_resolver_policy<Functor>::table_base<HashTable, ValueType, Allocator>::callback {
  friend table_base;

  public:
  callback(const std::shared_ptr<HashTable>& self, detail::refcount_ptr<ValueType, Allocator> value);
  callback(const callback&) = delete;
  ~callback();

  template<typename... Args>
  auto assign(Args&&... args) -> bool;
  auto assign_error(typename ValueType::error_type ex) noexcept -> bool;
  auto cancel() noexcept -> bool;

  private:
  const std::weak_ptr<HashTable> weak_self;
  detail::refcount_ptr<ValueType, Allocator> value;
  bool called = false;
  bool live = false;
};


} /* namespace libhoard */

#include "resolver_policy.ii"
