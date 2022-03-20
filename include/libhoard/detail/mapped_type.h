#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "function_ref.h"
#include "identity_fn.h"
#include "pending.h"

namespace libhoard::detail {


struct expired_t {};


template<typename T, typename Allocator, typename ErrorType>
class mapped_value {
  public:
  using mapped_type = T;
  using error_type = ErrorType;
  using pending_type = pending<mapped_type, Allocator, error_type>;
  using allocator_type = typename pending_type::allocator_type;
  using callback_fn = typename pending_type::callback_fn;

  private:
  using variant_type = std::variant<pending_type, mapped_type, expired_t, error_type>;

  template<typename Table, typename... Args, std::size_t... Indices>
  mapped_value(const Table& table, std::piecewise_construct_t pc, std::tuple<Args...> args, std::index_sequence<Indices...> indices) noexcept(std::is_nothrow_constructible_v<T, Args...>);

  public:
  template<typename Table>
  explicit mapped_value(const Table& table, allocator_type allocator = allocator_type());
  mapped_value(const mapped_value&) = delete;
  template<typename Table>
  mapped_value(const Table& table, std::piecewise_construct_t pc, error_type ex) noexcept(std::is_nothrow_move_constructible_v<ErrorType>);
  template<typename Table, typename... Args>
  mapped_value(const Table& table, std::piecewise_construct_t pc, std::tuple<Args...> args) noexcept(std::is_nothrow_constructible_v<T, Args...>);

  template<typename... Args>
  auto assign(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> void;
  auto assign_error(error_type ex) noexcept -> void;
  auto weaken() noexcept -> void;
  auto strengthen() noexcept -> bool;
  auto expired() const noexcept -> bool;
  auto pending() const noexcept -> bool;
  auto cancel() noexcept -> void;
  auto mark_expired() noexcept -> void;
  auto holds_error() const noexcept -> bool;
  auto holds_value() const noexcept -> bool;

  auto get_if_matching(function_ref<bool(const mapped_type&)> matcher) const -> std::variant<std::monostate, mapped_type, error_type>;
  auto get([[maybe_unused]] std::true_type include_pending) noexcept(std::is_nothrow_copy_constructible_v<mapped_type>) -> std::variant<std::monostate, mapped_type, error_type, pending_type*>;
  auto get([[maybe_unused]] std::false_type include_pending) const noexcept(std::is_nothrow_copy_constructible_v<mapped_type>) -> std::variant<std::monostate, mapped_type, error_type>;
  auto get_pending() noexcept -> pending_type*;
  auto matches(function_ref<bool(const mapped_type&)> matcher) const -> bool;

  private:
  variant_type value_;
};


template<typename Pointer, typename Allocator, typename ErrorType, typename WeakPointer, typename MemberPointer, typename MPCA>
class mapped_pointer {
  public:
  using mapped_type = Pointer;
  using error_type = ErrorType;
  using pending_type = pending<mapped_type, Allocator, error_type>;
  using allocator_type = typename pending_type::allocator_type;
  using callback_fn = typename pending_type::callback_fn;

  private:
  using variant_type = std::conditional_t<
      std::is_nothrow_default_constructible_v<WeakPointer>,
      std::variant<pending_type, MemberPointer, WeakPointer, error_type>,
      std::variant<pending_type, MemberPointer, WeakPointer, error_type, expired_t>>;
  struct mpca_tag {};

  template<typename Table, typename... Args, std::size_t... Indices>
  mapped_pointer(const Table& table, mpca_tag tag, std::tuple<Args...> args, std::index_sequence<Indices...> indices) noexcept(std::is_nothrow_constructible_v<MemberPointer, Args...>);

  template<typename Table, typename... Args>
  mapped_pointer(const Table& table, mpca_tag tag, std::tuple<Args...> args) noexcept(std::is_nothrow_constructible_v<MemberPointer, Args...>);

  public:
  template<typename Table>
  explicit mapped_pointer(const Table& table, allocator_type allocator = allocator_type());
  mapped_pointer(const mapped_pointer&) = delete;
  template<typename Table>
  mapped_pointer(const Table& table, std::piecewise_construct_t pc, error_type ex) noexcept(std::is_nothrow_move_constructible_v<ErrorType>);
  template<typename Table, typename... Args>
  mapped_pointer(const Table& table, std::piecewise_construct_t pc, std::tuple<Args...> args) noexcept(std::is_nothrow_constructible_v<MemberPointer, Args...>);

  template<typename... Args>
  auto assign(Args&&... args) noexcept(std::is_nothrow_constructible_v<MemberPointer, Args...> && std::is_nothrow_constructible_v<Pointer, MemberPointer> && std::is_nothrow_constructible_v<WeakPointer, Pointer>) -> void;
  auto assign_error(error_type ex) noexcept -> void;
  auto weaken() noexcept -> void;

  auto strengthen() -> bool;

  auto expired() const noexcept -> bool;
  auto pending() const noexcept -> bool;
  auto cancel() noexcept -> void;
  auto mark_expired() noexcept -> void;
  auto holds_error() const noexcept -> bool;
  auto holds_value() const noexcept -> bool;

  auto get_if_matching(function_ref<bool(const mapped_type&)> matcher) const -> std::variant<std::monostate, mapped_type, error_type>;
  auto get([[maybe_unused]] std::true_type include_pending) noexcept(std::is_nothrow_copy_constructible_v<mapped_type> && noexcept(std::declval<const WeakPointer&>().lock())) -> std::variant<std::monostate, mapped_type, error_type, pending_type*>;
  auto get([[maybe_unused]] std::false_type include_pending) const noexcept(std::is_nothrow_copy_constructible_v<mapped_type> && noexcept(std::declval<const WeakPointer&>().lock())) -> std::variant<std::monostate, mapped_type, error_type>;
  auto get_pending() noexcept -> pending_type*;
  auto matches(function_ref<bool(const mapped_type&)> matcher) const -> bool;

  private:
  MPCA mpca_;
  variant_type value_;
};


} /* namespace libhoard::detail */

#include "mapped_type.ii"
