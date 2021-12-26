#pragma once

#include <utility>
#include <cstddef>
#include <atomic>
#include <type_traits>
#include <memory>

namespace libhoard::detail {


class refcount;
template<typename T, typename Allocator> class refcount_ptr;


///\brief Increment reference counter in \p r
auto refcount_inc(const refcount* r) noexcept -> void;
///\brief Decrement reference counter in \p r
///\return True if the reference counter reached zero.
auto refcount_dec(const refcount* r) noexcept -> bool;

template<typename T, typename Allocator, typename... Args>
auto allocate_refcount(Allocator&& allocator, Args&&... args) -> refcount_ptr<T, std::decay_t<Allocator>>;


class refcount {
  friend auto refcount_inc(const refcount* r) noexcept -> void;
  friend auto refcount_dec(const refcount* r) noexcept -> bool;

  protected:
  refcount() noexcept;
  refcount(const refcount&) noexcept;
  refcount(refcount&&) noexcept;
  ~refcount() noexcept;
  auto operator=(const refcount&) noexcept -> refcount&;
  auto operator=(refcount&&) noexcept -> refcount&;

  private:
  mutable std::atomic<std::size_t> n_{ 0u };
};


/**
 * \brief A reference counted pointer, with an associated allocator.
 * \details
 * This pointer can share, and will destroy and deallocate the pointee using the supplied allocator.
 *
 * \tparam T The element type of this pointer.
 * \tparam Allocator Allocator that will be used to free \p T
 */
template<typename T, typename Allocator = std::allocator<T>>
class refcount_ptr {
  template<typename U, typename AllocatorU, typename... Args>
  friend auto allocate_refcount(AllocatorU&& allocator, Args&&... args) -> refcount_ptr<U, std::decay_t<AllocatorU>>;
  template<typename U, typename AllocatorU> friend class refcount_ptr;

  static_assert(std::is_convertible_v<std::remove_const_t<T>*, refcount*>,
      "T must derive (publicly, non-ambiguously) from refcount");
  static_assert(std::is_same_v<typename std::allocator_traits<Allocator>::value_type, std::remove_const_t<T>>,
      "Allocator must be an allocator of T");

  public:
  using element_type = T;
  using allocator_type = Allocator;

  private:
  refcount_ptr(element_type* ptr, const allocator_type& alloc) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>);
  refcount_ptr(element_type* ptr, allocator_type&& alloc) noexcept(std::is_nothrow_move_constructible_v<allocator_type>);

  public:
  explicit refcount_ptr(allocator_type alloc = allocator_type()) noexcept(std::is_nothrow_move_constructible_v<allocator_type>);
  explicit refcount_ptr([[maybe_unused]] std::nullptr_t nil, allocator_type alloc = allocator_type()) noexcept(std::is_nothrow_move_constructible_v<allocator_type>);
  refcount_ptr(const refcount_ptr& y) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>);
  refcount_ptr(refcount_ptr&& y) noexcept(std::is_nothrow_copy_constructible_v<allocator_type> || std::is_nothrow_move_constructible_v<allocator_type>);
  ~refcount_ptr() noexcept;

  template<typename U, typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_same_v<T, const U>>>
  refcount_ptr(const refcount_ptr<U, allocator_type>& y) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>);
  template<typename U, typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_same_v<T, const U>>>
  refcount_ptr(refcount_ptr<U, allocator_type>&& y) noexcept(std::is_nothrow_copy_constructible_v<allocator_type> || std::is_nothrow_move_constructible_v<allocator_type>);

  auto operator=(const refcount_ptr& y) noexcept(std::is_nothrow_copy_constructible_v<allocator_type> && std::is_nothrow_copy_assignable_v<allocator_type>) -> refcount_ptr&;
  auto operator=(refcount_ptr&& y) noexcept(std::is_nothrow_copy_constructible_v<allocator_type> && std::is_nothrow_copy_assignable_v<allocator_type>) -> refcount_ptr&;
  auto operator=(std::nullptr_t y) noexcept -> refcount_ptr&;
  auto adopt(element_type* ptr) noexcept -> refcount_ptr&;

  auto get_allocator() const noexcept(std::is_nothrow_copy_constructible_v<allocator_type>) -> allocator_type;
  auto get() const noexcept -> element_type*;
  auto operator*() const noexcept -> element_type&;
  auto operator->() const noexcept -> element_type*;
  auto release() noexcept -> element_type*;
  auto reset() -> void;
  explicit operator bool() const noexcept;

  auto operator==(std::nullptr_t nil) const noexcept -> bool;
  auto operator!=(std::nullptr_t nil) const noexcept -> bool;
  auto operator==(const refcount_ptr& y) const noexcept -> bool;
  auto operator!=(const refcount_ptr& y) const noexcept -> bool;

  template<typename U, typename AllocatorU>
  auto operator==(const refcount_ptr<U, AllocatorU>& y) const noexcept -> bool;
  template<typename U, typename AllocatorU>
  auto operator!=(const refcount_ptr<U, AllocatorU>& y) const noexcept -> bool;

  private:
  // Invokes refcount_inc, avoiding ADL.
  static auto inc_(const refcount* r) noexcept -> void;
  // Invokes refcount_dec, avoiding ADL.
  static auto dec_(const refcount* r) noexcept -> bool;

  allocator_type alloc_;
  element_type* ptr_ = nullptr;
};


} /* namespace libhoard::detail */

#include "refcount.ii"
