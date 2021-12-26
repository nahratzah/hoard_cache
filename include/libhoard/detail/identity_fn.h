#pragma once

#include <type_traits>

namespace libhoard::detail {


/**
 * \brief Function that returns the passed in argument.
 * \details
 * Tiny function that passes in the argument it's given.
 *
 * \note Rvalue-references are converted to non-references.
 * This means it is moved from.
 * All other references have their reference property preserved.
 */
struct identity_fn {
  ///\brief Identity functor.
  ///\param v Value that is returned by this functor.
  ///\tparam T The type that is passed in. If \p T is an rvalue, it must be move-constructible.
  template<typename T>
  auto operator()(T&& v) const noexcept(std::is_lvalue_reference_v<T> || std::is_nothrow_constructible_v<std::remove_reference_t<T>, T>) -> T;
};


} /* namespace libhoard::detail */

#include "identity_fn.ii"
