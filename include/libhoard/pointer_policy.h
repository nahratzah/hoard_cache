#pragma once

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace libhoard {
namespace detail {


template<typename Pointer, typename WeakPointer>
struct weak_pointer_check_ {
  static_assert(std::is_constructible_v<WeakPointer, std::add_rvalue_reference_t<Pointer>>,
      "weak-pointer must be constructible from pointer");
  static_assert(std::is_constructible_v<Pointer, decltype(std::declval<const WeakPointer&>().lock())>,
      "weak-pointer::lock() must result in a regular pointer");

  using type = WeakPointer;
};

template<typename Pointer>
struct weak_pointer_check_<Pointer, void>
: weak_pointer_check_<Pointer, typename Pointer::weak_type>
{};


template<typename Pointer, typename MemberPointer>
struct member_pointer_check_ {
  static_assert(std::is_constructible_v<Pointer, const MemberPointer&>,
      "pointer must be constructible from member-pointer");

  using type = MemberPointer;
};

template<typename Pointer>
struct member_pointer_check_<Pointer, void>
: member_pointer_check_<Pointer, Pointer>
{};


struct default_member_pointer_constructor_args {
  template<typename Pointer>
  auto operator()(Pointer&& ptr) const noexcept -> std::tuple<std::add_rvalue_reference_t<Pointer>> {
    return std::tuple<std::add_rvalue_reference_t<Pointer>>(std::forward<Pointer>(ptr));
  }
};


template<typename T, typename Tuple>
struct is_tuple_constructible_;

template<typename T, typename... TupleArgs>
struct is_tuple_constructible_<T, std::tuple<TupleArgs...>&&>
: std::is_constructible<T, std::add_rvalue_reference_t<TupleArgs>...>
{};

template<typename T, typename... TupleArgs>
struct is_tuple_constructible_<T, std::tuple<TupleArgs...>&>
: std::is_constructible<T, std::add_lvalue_reference_t<TupleArgs>...>
{};

template<typename T, typename... TupleArgs>
struct is_tuple_constructible_<T, const std::tuple<TupleArgs...>&>
: std::is_constructible<T, std::add_lvalue_reference_t<std::add_const_t<TupleArgs>>...>
{};

template<typename T, typename... TupleArgs>
struct is_tuple_constructible_<T, std::tuple<TupleArgs...>>
: is_tuple_constructible_<T, std::tuple<TupleArgs...>&&>
{};

template<typename T, typename Tuple>
using is_tuple_constructible = typename is_tuple_constructible_<T, Tuple>::type;

template<typename T, typename Tuple>
inline constexpr bool is_tuple_constructible_v = is_tuple_constructible<T, Tuple>::value;


template<typename Pointer, typename MemberPointer, typename MemberPointerConstructorArgs>
struct member_pointer_constructor_args {
  static_assert(!std::is_same_v<MemberPointerConstructorArgs, default_member_pointer_constructor_args> ||
      std::is_constructible_v<MemberPointer, Pointer&&>,
      "member-pointer must be constructible from pointer");
  static_assert(std::is_invocable_v<MemberPointerConstructorArgs&, Pointer>,
      "the functor must accept a pointer argument");
  static_assert(std::is_invocable_v<MemberPointerConstructorArgs&, Pointer> ?
      std::is_invocable_v<const MemberPointerConstructorArgs&, Pointer> :
      /* not being able to invoke the functor has been handled by previous static-assert, so we silence this assertion */ true,
      "the functor the computes the constructor arguments for the member-pointer, must be const-invocable");

  using tuple_type = decltype(std::declval<const MemberPointerConstructorArgs&>()(std::declval<Pointer>()));

  static_assert(is_tuple_constructible_v<MemberPointer, std::add_rvalue_reference_t<tuple_type>>,
      "member-pointer must be constructible using the functor tuple result");

  using type = MemberPointerConstructorArgs;
};


} /* namespace libhoard::detail */


template<typename WeakPointer = void, typename MemberPointer = void, typename MemberPointerConstructorArgs = detail::default_member_pointer_constructor_args>
class pointer_policy {
  public:
  // Contains the resolved weak-pointer type.
  // Also ensures the weak-pointer type has been verified, for easier error messages.
  template<typename MappedPointer>
  using weak_type = typename detail::weak_pointer_check_<MappedPointer, WeakPointer>::type;

  // Contains the resolved member-pointer type.
  // Also ensures the member-pointer type has been verified, for easier error messages.
  template<typename MappedPointer>
  using member_type = typename detail::member_pointer_check_<MappedPointer, MemberPointer>::type;

  // Figure out the constructor arguments for the member-pointer.
  template<typename MappedPointer>
  using member_pointer_constructor_args = typename detail::member_pointer_constructor_args<MappedPointer, member_type<MappedPointer>, MemberPointerConstructorArgs>::type;

  class mapped_base;
  class table_base_impl;

  template<typename HashTable, typename ValueType, typename Allocator>
  using table_base = table_base_impl;

  explicit pointer_policy(MemberPointerConstructorArgs mpca_fn = MemberPointerConstructorArgs())
  : mpca_fn_(std::move(mpca_fn))
  {}

  private:
  MemberPointerConstructorArgs mpca_fn_;
};


template<typename WeakPointer, typename MemberPointer, typename MemberPointerConstructorArgs>
class pointer_policy<WeakPointer, MemberPointer, MemberPointerConstructorArgs>::table_base_impl {
  friend mapped_base;

  public:
  template<typename... Args, typename Alloc>
  table_base_impl(const std::tuple<Args...>& args, [[maybe_unused]] const Alloc& alloc)
  : mpca_fn_(std::get<pointer_policy>(args).mpca_fn_)
  {}

  private:
  MemberPointerConstructorArgs mpca_fn_;
};


template<typename WeakPointer, typename MemberPointer, typename MemberPointerConstructorArgs>
class pointer_policy<WeakPointer, MemberPointer, MemberPointerConstructorArgs>::mapped_base {
  public:
  explicit mapped_base(const table_base_impl& table)
  : mpca_fn_(table.mpca_fn_)
  {}

  template<typename Pointer>
  auto strengthen_args_(Pointer&& pointer) -> decltype(std::invoke(std::declval<const MemberPointerConstructorArgs&>(), std::declval<Pointer>())) {
    return std::invoke(mpca_fn_, std::forward<Pointer>(pointer));
  }

  template<typename Pointer>
  static auto mpca_args_static_(const table_base_impl& table, Pointer&& pointer) -> decltype(std::invoke(std::declval<const MemberPointerConstructorArgs&>(), std::declval<Pointer>())) {
    return std::invoke(table.mpca_fn_, std::forward<Pointer>(pointer));
  }

  private:
  MemberPointerConstructorArgs mpca_fn_;
};


} /* namespace libhoard */
