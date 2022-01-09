#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "function_ref.h"
#include "identity.h"

namespace libhoard::detail {


template<typename KeyType, typename Mapper, typename... BaseTypes>
class value_type
: public BaseTypes...
{
  private:
  using mapper = Mapper;

  public:
  using key_type = KeyType;
  using mapped_type = typename mapper::mapped_type;
  using error_type = typename mapper::error_type;
  using allocator_type = typename mapper::allocator_type;
  using pending_type = typename mapper::pending_type;
  using callback_fn = typename mapper::callback_fn;

  private:
  const key_type key_;
  mapper mapped_;

  template<typename HashTable, typename... Args, std::size_t... KeyIndices>
  value_type(const HashTable& table, std::piecewise_construct_t pc, std::tuple<Args...> key, std::index_sequence<KeyIndices...> key_indices);

  template<typename HashTable, typename... Args, std::size_t... KeyIndices>
  value_type(const HashTable& table, std::piecewise_construct_t pc, std::tuple<Args...> key, std::index_sequence<KeyIndices...> key_indices, error_type ex);

  template<typename HashTable, typename... Args, typename... MappedArgs, std::size_t... KeyIndices>
  value_type(const HashTable& table, std::piecewise_construct_t pc, std::tuple<Args...> key, std::index_sequence<KeyIndices...> key_indices, std::tuple<MappedArgs...> mapped_args);

  public:
  template<typename HashTable>
  value_type(const HashTable& table, key_type key);

  template<typename HashTable, typename... Args>
  value_type(const HashTable& table, std::piecewise_construct_t pc, std::tuple<Args...> args);

  template<typename HashTable, typename... Args>
  value_type(const HashTable& table, std::piecewise_construct_t pc, std::tuple<Args...> args, error_type ex);

  template<typename HashTable, typename... Args, typename... MappedArgs>
  value_type(const HashTable& table, std::piecewise_construct_t pc, std::tuple<Args...> args, std::tuple<MappedArgs...> mapped_args);

  value_type(const value_type&) = delete;
  value_type(value_type&&) = delete;

  auto get_if_matching(function_ref<bool(const key_type&)> matcher, std::false_type include_pending) const -> std::variant<std::monostate, mapped_type, error_type>;
  auto get_if_matching(function_ref<bool(const key_type&)> matcher, std::true_type include_pending) -> std::variant<std::monostate, mapped_type, error_type, pending_type*>;
  auto matches(function_ref<bool(const key_type&)> matcher) const -> bool;
  auto get(std::false_type include_pending) const -> std::variant<std::monostate, mapped_type, error_type>;
  auto get(std::true_type include_pending) -> std::variant<std::monostate, mapped_type, error_type, pending_type*>;

  auto pending() const noexcept -> bool;
  auto expired() const noexcept -> bool;
  auto mark_expired() noexcept -> void;
  auto weaken() noexcept -> void;
  auto strengthen() noexcept -> bool;
  auto cancel() noexcept -> void;
  auto holds_value() const noexcept -> bool;
  auto holds_error() const noexcept -> bool;
  auto get_pending() noexcept -> pending_type*;
  auto key() const -> std::optional<key_type>;

  template<typename... Args>
  auto assign(Args&&... args) noexcept(noexcept(std::declval<mapper&>().assign(std::declval<Args>()...))) -> void;
  auto assign_error(error_type ex) noexcept -> void;
};

template<typename Mapper, typename... BaseTypes>
class value_type<identity_t, Mapper, BaseTypes...>
: public BaseTypes...
{
  private:
  using mapper = Mapper;

  public:
  using mapped_type = typename mapper::mapped_type;
  using key_type = mapped_type;
  using error_type = typename mapper::error_type;
  using allocator_type = typename mapper::allocator_type;
  using pending_type = typename mapper::pending_type;
  using callback_fn = typename mapper::callback_fn;

  private:
  mapper mapped_;

  public:
  template<typename HashTable>
  explicit value_type(const HashTable& table, key_type key);

  template<typename HashTable, typename... Args>
  explicit value_type(const HashTable& table, std::piecewise_construct_t pc, std::tuple<Args...> args);

  value_type(const value_type&) = delete;
  value_type(value_type&&) = delete;

  auto get_if_matching(function_ref<bool(const key_type&)> matcher, std::false_type include_pending) const -> std::variant<std::monostate, mapped_type, error_type>;
  auto matches(function_ref<bool(const key_type&)> matcher) const -> bool;
  auto get(std::false_type include_pending) const -> std::variant<std::monostate, mapped_type, error_type>;
  auto get(std::true_type include_pending) -> std::variant<std::monostate, mapped_type, error_type, pending_type*>;

  auto pending() const noexcept -> bool;
  auto expired() const noexcept -> bool;
  auto mark_expired() noexcept -> void;
  auto weaken() noexcept -> void;
  auto strengthen() noexcept -> bool;
  auto cancel() noexcept -> void;
  auto holds_value() const noexcept -> bool;
  auto holds_error() const noexcept -> bool;
  auto get_pending() noexcept -> pending_type*;
  auto key() const -> std::optional<key_type>;
};


} /* namespace libhoard::detail */

#include "value_type.ii"
