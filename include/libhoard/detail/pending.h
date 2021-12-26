#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace libhoard::detail {


///\brief Pending completion callbacks.
template<typename T, typename Allocator, typename ErrorType>
class [[nodiscard("must invoke pending callbacks")]] pending {
  public:
  using value_type = T;
  using error_type = ErrorType;
  using callback_fn = std::function<void(const value_type&, const error_type&)>;
  using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<callback_fn>;

  private:
  using callbacks = std::vector<callback_fn, allocator_type>;

  public:
  explicit pending(allocator_type alloc = allocator_type());
  pending(pending&& y) noexcept = default;
  pending(const pending&) = delete;

  auto add_callback(callback_fn cb) -> void;
  auto resolve_success(const value_type& v) noexcept -> void;
  auto resolve_failure(const error_type& ex) noexcept -> void;
  auto expired() const noexcept -> bool;
  auto weakened() const noexcept -> bool;
  auto weaken() noexcept -> void;
  auto strengthen() noexcept -> bool;
  auto cancel() noexcept -> void;
  auto mark_expired() noexcept -> void;

  private:
  callbacks cbq_;
  bool expired_ : 1;
  bool weakened_ : 1;
};


} /* namespace libhoard::detail */

#include "pending.ii"
