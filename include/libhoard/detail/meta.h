#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace libhoard::detail {


template<typename... Types> struct type_list; // forward declaration


template<typename, typename, typename...> struct type_list_remove_all_;

template<typename... Types, typename ToRemove>
struct type_list_remove_all_<type_list<Types...>, ToRemove> {
  using type = type_list<Types...>;
};

template<typename... Types, typename ToRemove, typename T0, typename... Ts>
struct type_list_remove_all_<type_list<Types...>, ToRemove, T0, Ts...>
: type_list_remove_all_<
    std::conditional_t<
        std::is_same_v<ToRemove, T0>,
        type_list<Types...>,
        type_list<Types..., T0>>,
    ToRemove, Ts...>
{};


template<typename, typename, typename...> struct type_list_remove_;

template<typename... Types, typename ToRemove>
struct type_list_remove_<type_list<Types...>, ToRemove> {
  using type = type_list<Types...>;
};

template<typename... Types, typename ToRemove, typename T0, typename... Ts>
struct type_list_remove_<type_list<Types...>, ToRemove, T0, Ts...> {
  using type = std::conditional_t<
      std::is_same_v<ToRemove, T0>,
      type_list<Types..., Ts...>,
      typename type_list_remove_<type_list<Types..., T0>, ToRemove, Ts...>::type>;
};


template<typename, typename...> struct type_list_distinct_;

template<typename... Types>
struct type_list_distinct_<type_list<Types...>> {
  using type = type_list<Types...>;
};

template<typename... Types, typename T0, typename... Ts>
struct type_list_distinct_<type_list<Types...>, T0, Ts...>
: std::conditional_t<
      std::disjunction_v<std::is_same<T0, Types>...>,
      type_list_distinct_<type_list<Types...>, Ts...>,
      type_list_distinct_<type_list<Types..., T0>, Ts...>>
{};


template<typename, typename...> struct type_list_extend_;

template<typename... Types>
struct type_list_extend_<type_list<Types...>> {
  using type = type_list<Types...>;
};

template<typename... Types, typename... AddTypes, typename... U>
struct type_list_extend_<type_list<Types...>, type_list<AddTypes...>, U...>
: type_list_extend_<type_list<Types..., AddTypes...>, U...>
{};


template<typename, typename...> struct type_list_reverse_;

template<typename... Types>
struct type_list_reverse_<type_list<Types...>> {
  using type = type_list<Types...>;
};

template<typename... Types, typename T0, typename... Ts>
struct type_list_reverse_<type_list<Types...>, T0, Ts...>
: type_list_reverse_<type_list<T0, Types...>, Ts...>
{};


template<typename, template<typename> class, typename...> struct type_list_filter_;

template<typename... Types, template<typename> class Filter>
struct type_list_filter_<type_list<Types...>, Filter> {
  using type = type_list<Types...>;
};

template<typename... Types, template<typename> class Filter, typename T0, typename... Ts>
struct type_list_filter_<type_list<Types...>, Filter, T0, Ts...>
: std::conditional_t<
      Filter<T0>::value,
      type_list_filter_<type_list<Types..., T0>, Filter, Ts...>,
      type_list_filter_<type_list<Types...>, Filter, Ts...>>
{};


template<typename, typename...> struct type_list_exclude_;

template<typename... Types>
struct type_list_exclude_<type_list<Types...>> {
  using type = type_list<Types...>;
};

template<typename... Types, typename... U>
struct type_list_exclude_<type_list<Types...>, type_list<>, U...>
: type_list_exclude_<type_list<Types...>, U...>
{};

template<typename... Types, typename T0, typename... Ts, typename... U>
struct type_list_exclude_<type_list<Types...>, type_list<T0, Ts...>, U...>
: type_list_exclude_<typename type_list<Types...>::template remove_all_t<T0>, type_list<Ts...>, U...>
{};


template<typename Functor, typename T, typename = void>
struct apply_for_each_type_invoker_ {
  auto invoke(Functor& functor) const noexcept -> void {}
};

template<typename Functor, typename T>
struct apply_for_each_type_invoker_<Functor, T, std::void_t<decltype(std::invoke(std::declval<Functor&>(), std::declval<T*>()))>> {
  auto invoke(Functor& functor) const noexcept(noexcept(std::invoke(std::declval<Functor&>(), std::declval<T*>()))) -> void {
    T* nil = nullptr;
    std::invoke(functor, nil);
  }
};


///\brief Invoke a functor for each type, where functor(T*) is invocable.
template<typename... Ts> struct maybe_apply_for_each_type;

template<>
struct maybe_apply_for_each_type<> {
  public:
  template<typename Functor>
  auto operator()([[maybe_unused]] Functor&& functor) const -> void {
    // SKIP
  }
};

template<typename T0, typename... Ts>
struct maybe_apply_for_each_type<T0, Ts...> {
  public:
  template<typename Functor>
  auto operator()(Functor&& functor) const {
    apply_for_each_type_invoker_<std::remove_reference_t<Functor>, T0>().invoke(functor);
    std::invoke(next_, std::forward<Functor>(functor));
  }

  private:
  maybe_apply_for_each_type<Ts...> next_;
};


template<typename... Types> struct type_list {
  ///\brief Add zero or more types to the list.
  template<typename... T>
  using add_type_t = type_list<Types..., T...>;

  ///\brief Extend the list with other lists.
  template<typename... U>
  using extend_t = typename type_list_extend_<type_list<Types...>, U...>::type;

  ///\brief Test if a specific type is present.
  template<typename T>
  using has_type_t = std::disjunction<std::is_same<T, Types>...>;
  template<typename T>
  static inline constexpr bool has_type_v = has_type_t<T>::value;

  ///\brief Remove a specific type. Only one instance of the type is removed.
  template<typename T>
  using remove_t = typename type_list_remove_<type_list<>, T, Types...>::type;

  ///\brief Remove all instances of a specific type.
  template<typename T>
  using remove_all_t = typename type_list_remove_all_<type_list<>, T, Types...>::type;

  ///\brief Remove any duplicates in the list.
  using distinct_t = typename type_list_distinct_<type_list<>, Types...>::type;

  ///\brief Reverse the order of types in the list.
  using reverse_t = typename type_list_reverse_<type_list<>, Types...>::type;

  ///\brief Filter the types, only retaining the ones for which Filter<T>::value is true.
  template<template<typename> class Filter>
  using filter_t = typename type_list_filter_<type_list<>, Filter, Types...>::type;

  ///\brief Apply the types to a template class.
  template<template<typename...> class Template>
  using apply_t = Template<Types...>;

  ///\brief Apply transformation to each type, and substitute the transformed types `type` member-type.
  template<template<typename> class Transformer>
  using transform_t = type_list<typename Transformer<Types>::type...>;

  ///\brief Remove all members from type_list U.
  ///\tparam U Zero or more type_lists containing types that are to be removed.
  template<typename... U>
  using exclude_t = typename type_list_exclude_<type_list<Types...>, U...>::type;

  static inline constexpr std::size_t size = sizeof...(Types);
  static inline constexpr bool empty = (size == 0);
};


} /* namespace libhoard::detail */
