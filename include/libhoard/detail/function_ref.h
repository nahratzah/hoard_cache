#pragma once

#include <type_traits>

namespace libhoard::detail {


/**
 * \brief Function reference.
 * \details
 * This class functions a bit like std::function.
 * But instead of making a local copy of the function, instead
 * we create a wrap a reference to the function.
 *
 * The advantage is that we gain a much more light-weight implementation
 * (a trivially copy constructible pair of pointers),
 * and can accept non-copyable function references.
 *
 * A specialization for noexcept-functions is supplied.
 *
 * \tparam Fun A function prototype, of the form `R (Arg1, Arg2, ..., ArgN)`.
 */
template<typename Fun> class function_ref;

template<typename R, typename... Args>
class function_ref<R(Args...)> {
  private:
  using internal_fun = R(void*, Args...);

  public:
  using result_type = R;

  template<typename Fn>
  function_ref(Fn&& fn) noexcept;

  auto operator()(Args... args) const -> R;

  private:
  internal_fun* internal_;
  void* ref_;
};

template<typename R, typename... Args>
class function_ref<R(Args...) noexcept> {
  private:
  using internal_fun = R(void*, Args...) noexcept;

  public:
  using result_type = R;

  template<typename Fn>
  function_ref(Fn&& fn) noexcept;

  auto operator()(Args... args) const noexcept -> R;

  private:
  internal_fun* internal_;
  void* ref_;
};


static_assert(!std::is_default_constructible_v<function_ref<void()>>);
static_assert(!std::is_default_constructible_v<function_ref<void() noexcept>>);


} /* namespace libhoard::detail */

#include "function_ref.ii"
