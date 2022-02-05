#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "function_ref.h"

namespace libhoard::detail {


template<typename Allocator>
class basic_hashtable_allocator_member {
  public:
  using allocator_type = Allocator;

  protected:
  explicit basic_hashtable_allocator_member(allocator_type allocator = allocator_type());
  basic_hashtable_allocator_member(const basic_hashtable_allocator_member&) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>) = default;
  basic_hashtable_allocator_member(basic_hashtable_allocator_member&&) noexcept(std::is_nothrow_move_constructible_v<allocator_type>) = default;
  ~basic_hashtable_allocator_member() noexcept = default;

  public:
  auto get_allocator() const noexcept(std::is_nothrow_copy_constructible_v<allocator_type>) -> allocator_type;

  protected:
  allocator_type alloc;
};


class basic_hashtable_algorithms {
  public:
  class element;
  using element_ptr = element*;
  using successor_ptr = element_ptr*;
  using size_type = std::uintptr_t;
  class iterator;
  class const_iterator;

  protected:
  basic_hashtable_algorithms() noexcept;
  basic_hashtable_algorithms(const basic_hashtable_algorithms&) = delete;
  basic_hashtable_algorithms(basic_hashtable_algorithms&& y) noexcept;
  ~basic_hashtable_algorithms() noexcept = default;

  public:
  static auto is_hashtable_linked(const element* elem) noexcept -> bool;
  auto empty() const noexcept -> bool;
  auto size() const noexcept -> size_type;
  auto bucket_count() const noexcept -> size_type;
  auto load_factor() const noexcept -> float;

  auto clear() noexcept -> void;

  template<typename DisposeCb>
  auto clear_and_dispose(DisposeCb&& dispose_cb) noexcept(std::is_nothrow_invocable_v<DisposeCb, element*>) -> void;

  auto unlink(const_iterator predecessor) noexcept -> element*;

  template<typename DisposeCb>
  auto unlink_and_dispose(const_iterator predecessor, DisposeCb&& dispose_cb) noexcept(std::is_nothrow_invocable_v<DisposeCb, element*>) -> void;

  auto begin() noexcept -> iterator;
  auto end() noexcept -> iterator;
  auto begin() const noexcept -> const_iterator;
  auto end() const noexcept -> const_iterator;
  auto cbegin() const noexcept -> const_iterator;
  auto cend() const noexcept -> const_iterator;

  auto before_begin() noexcept -> iterator;
  auto before_end() noexcept -> iterator;
  auto before_begin() const noexcept -> const_iterator;
  auto before_end() const noexcept -> const_iterator;
  auto cbefore_begin() const noexcept -> const_iterator;
  auto cbefore_end() const noexcept -> const_iterator;

  auto begin(size_type bucket_idx) noexcept -> iterator;
  auto end(size_type bucket_idx) noexcept -> iterator;
  auto begin(size_type bucket_idx) const noexcept -> const_iterator;
  auto end(size_type bucket_idx) const noexcept -> const_iterator;
  auto cbegin(size_type bucket_idx) const noexcept -> const_iterator;
  auto cend(size_type bucket_idx) const noexcept -> const_iterator;

  auto before_begin(size_type bucket_idx) noexcept -> iterator;
  auto before_end(size_type bucket_idx) noexcept -> iterator;
  auto before_begin(size_type bucket_idx) const noexcept -> const_iterator;
  auto before_end(size_type bucket_idx) const noexcept -> const_iterator;
  auto cbefore_begin(size_type bucket_idx) const noexcept -> const_iterator;
  auto cbefore_end(size_type bucket_idx) const noexcept -> const_iterator;

  protected:
  ///\return Iterator to the predecessor of the inserted element.
  auto link_(std::size_t hash, element* elem) noexcept -> iterator;
  auto rehash_(successor_ptr* buckets, size_type bucket_count) noexcept -> std::tuple<successor_ptr*, size_type>;
  auto release_buckets_() noexcept -> std::tuple<successor_ptr*, size_type>;

  public:
  auto bucket_for(std::size_t hash) const noexcept -> size_type;
  auto bucket_for(const element* e) const noexcept -> size_type;

  private:
  auto unlink_(element** pred_ptr) noexcept -> element*;

  element_ptr head_ = nullptr; // points at the first element
  successor_ptr* buckets_ = nullptr; // array of pointers
  size_type bucket_count_ = 0; // number of element in buckets_ array
  size_type size_ = 0;
};


class basic_hashtable_algorithms::element {
  friend basic_hashtable_algorithms;
  friend basic_hashtable_algorithms::iterator;
  friend basic_hashtable_algorithms::const_iterator;

  protected:
  element() noexcept = default;
  element(const element&) noexcept {}
  element(element&&) noexcept {}
  ~element() noexcept = default;
  auto operator=(const element&) noexcept -> element& { return *this; }
  auto operator=(element&&) noexcept -> element& { return *this; }

  public:
  auto hash() const noexcept -> std::size_t { return hash_; }

  private:
  element_ptr successor_ = nullptr;
  std::size_t hash_;
};


class basic_hashtable_algorithms::iterator {
  friend basic_hashtable_algorithms;
  friend basic_hashtable_algorithms::const_iterator;

  public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = element;
  using reference = element&;
  using pointer = element*;
  using difference_type = std::intptr_t;

  iterator() noexcept = default;

  private:
  explicit iterator(element_ptr* s) noexcept;

  public:
  auto operator++() noexcept -> iterator&;
  auto operator++(int) noexcept -> iterator;
  auto operator*() const noexcept -> element&;
  auto operator->() const noexcept -> element*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  element_ptr* s_;
};


class basic_hashtable_algorithms::const_iterator {
  friend basic_hashtable_algorithms;
  friend basic_hashtable_algorithms::iterator;

  public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = element;
  using reference = element&;
  using pointer = element*;
  using difference_type = std::intptr_t;

  const_iterator() noexcept = default;
  const_iterator(const iterator& y) noexcept;

  private:
  explicit const_iterator(const element_ptr* s) noexcept;

  public:
  auto operator++() noexcept -> const_iterator&;
  auto operator++(int) noexcept -> const_iterator;
  auto operator*() const noexcept -> const element&;
  auto operator->() const noexcept -> const element*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  const element_ptr* s_;
};


using basic_hashtable_element = basic_hashtable_algorithms::element;


template<typename Allocator = std::allocator<basic_hashtable_element*>>
class basic_hashtable
: public basic_hashtable_allocator_member<typename std::allocator_traits<Allocator>::template rebind_alloc<basic_hashtable_algorithms::successor_ptr>>,
  public basic_hashtable_algorithms
{
  private:
  static inline constexpr basic_hashtable_algorithms::size_type initial_size = 4;

  public:
  static_assert(std::is_same_v<typename std::allocator_traits<Allocator>::value_type, basic_hashtable_element*>,
      "allocator must allocate elements of basic_hashtable_element*");

  protected:
  explicit basic_hashtable(typename basic_hashtable::allocator_type allocator = typename basic_hashtable::allocator_type());
  explicit basic_hashtable(float max_load_factor, typename basic_hashtable::allocator_type allocator = typename basic_hashtable::allocator_type());
  ~basic_hashtable() noexcept;

  public:
  auto max_bucket_count() const noexcept -> size_type;
  auto max_load_factor() const noexcept -> float;
  auto max_load_factor(float new_max_load_factor) -> void;
  ///\return Iterator to the predecessor of the inserted element.
  auto link(std::size_t hash, element* elem, std::optional<function_ref<void()>> before_rehash = std::nullopt) -> basic_hashtable_algorithms::iterator;
  auto reserve(size_type new_size, std::optional<function_ref<void()>> before_rehash = std::nullopt) -> void;

  private:
  auto maybe_rehash_before_link_(std::optional<function_ref<void()>> before_rehash) -> void;
  auto rehash_(size_type new_bucket_count) -> void;
  float max_load_factor_ = 1.0;
};


} /* namespace libhoard::detail */

#include "basic_hashtable.ii"
