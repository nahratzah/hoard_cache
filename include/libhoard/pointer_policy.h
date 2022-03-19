#pragma once

#include <type_traits>

namespace libhoard {
namespace detail {


template<typename Pointer, typename WeakPointer>
struct weak_pointer_check_ {
  static_assert(std::is_constructible_v<WeakPointer, std::add_rvalue_reference_t<Pointer>>,
      "weak-pointer must be constructible from pointer");
  static_assert(std::is_constructible_v<Pointer, decltype(std::declval<WeakPointer&>().strengthen())>,
      "weak-pointer::lock() must result in a regular pointer");

  using type = WeakPointer;
};

template<typename Pointer>
struct weak_pointer_check_<Pointer, void>
: weak_pointer_check_<Pointer, typename Pointer::weak_type>
{};


} /* namespace libhoard::detail */


template<typename WeakPointer = void, typename MemberPointer = void>
class pointer_policy {
  public:
  // Contains the resolved weak-pointer type.
  // Also ensures the weak-pointer type has been verified, for easier error messages.
  template<typename MappedPointer>
  using weak_type = typename detail::weak_pointer_check_<MappedPointer, WeakPointer>::type;

  // Contains the resolved member-pointer type.
  // Also ensures the member-pointer type has been verified, for easier error messages.
  template<typename MappedPointer>
  using member_type = MappedPointer; // XXX
};


} /* namespace libhoard */
