#pragma once

#include <cstddef>
#include <iterator>

namespace libhoard::detail {


class basic_linked_list {
  public:
  class link {
    friend basic_linked_list;

    protected:
    link() noexcept;
    link(const link& y) noexcept;
    link(link&& y) noexcept;
    ~link() noexcept;
    auto operator=(const link& y) noexcept -> link&;
    auto operator=(link&& y) noexcept -> link&;

    private:
    constexpr link(link* pred, link* succ) noexcept;

    link* succ_;
    link* pred_;
  };

  class iterator;
  class const_iterator;

  basic_linked_list() noexcept;
  basic_linked_list(const basic_linked_list&) = delete;
  basic_linked_list(basic_linked_list&& y) noexcept;

  auto link_front(link* elem) noexcept -> iterator;
  auto link_back(link* elem) noexcept -> iterator;
  auto link_after(const_iterator pos, link* elem) noexcept -> iterator;
  auto link_before(const_iterator pos, link* elem) noexcept -> iterator;
  auto unlink(const_iterator iter) noexcept -> iterator;

  auto empty() const noexcept -> bool;
  static auto iterator_to(link* v) noexcept -> iterator;
  static auto iterator_to(const link* v) noexcept -> const_iterator;

  auto begin() noexcept -> iterator;
  auto end() noexcept -> iterator;
  auto begin() const noexcept -> const_iterator;
  auto end() const noexcept -> const_iterator;
  auto cbegin() const noexcept -> const_iterator;
  auto cend() const noexcept -> const_iterator;

  private:
  auto link_(link* predecessor, link* elem, link* successor) noexcept -> void;

  link head_;
};


class basic_linked_list::iterator {
  friend basic_linked_list;
  friend basic_linked_list::const_iterator;

  public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = link;
  using reference = link&;
  using pointer = link*;
  using difference_type = std::ptrdiff_t;

  iterator() noexcept;

  private:
  explicit iterator(link* ptr) noexcept;

  public:
  auto operator++() noexcept -> iterator&;
  auto operator++(int) noexcept -> iterator;
  auto operator--() noexcept -> iterator&;
  auto operator--(int) noexcept -> iterator;

  auto get() const noexcept -> link*;
  auto operator*() const noexcept -> link&;
  auto operator->() const noexcept -> link*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  link* ptr_;
};


class basic_linked_list::const_iterator {
  friend basic_linked_list;
  friend basic_linked_list::iterator;

  public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = link;
  using reference = const link&;
  using pointer = const link*;
  using difference_type = std::ptrdiff_t;

  const_iterator() noexcept;
  const_iterator(const iterator& y) noexcept;

  private:
  explicit const_iterator(const link* ptr) noexcept;

  public:
  auto operator++() noexcept -> const_iterator&;
  auto operator++(int) noexcept -> const_iterator;
  auto operator--() noexcept -> const_iterator&;
  auto operator--(int) noexcept -> const_iterator;

  auto get() const noexcept -> const link*;
  auto operator*() const noexcept -> const link&;
  auto operator->() const noexcept -> const link*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  const link* ptr_;
};


template<typename T>
class linked_list {
  public:
  using value_type = T;
  class iterator;
  class const_iterator;

  auto link_front(value_type* elem) noexcept -> iterator;
  auto link_back(value_type* elem) noexcept -> iterator;
  auto link_before(const_iterator pos, value_type* elem) noexcept -> iterator;
  auto link_after(const_iterator pos, value_type* elem) noexcept -> iterator;
  auto unlink(const_iterator iter) noexcept -> iterator;

  auto begin() noexcept -> iterator;
  auto end() noexcept -> iterator;
  auto begin() const noexcept -> const_iterator;
  auto end() const noexcept -> const_iterator;
  auto cbegin() const noexcept -> const_iterator;
  auto cend() const noexcept -> const_iterator;

  auto empty() const noexcept -> bool;
  static auto iterator_to(value_type* v) noexcept -> iterator;
  static auto iterator_to(const value_type* v) noexcept -> const_iterator;

  private:
  basic_linked_list impl_;
};


template<typename T>
class linked_list<T>::iterator {
  friend linked_list;
  friend const_iterator;

  public:
  using iterator_category = std::iterator_traits<basic_linked_list::iterator>::iterator_category;
  using value_type = T;
  using reference = T&;
  using pointer = T*;
  using difference_type = std::iterator_traits<basic_linked_list::iterator>::difference_type;

  iterator() noexcept;

  private:
  explicit iterator(basic_linked_list::iterator iter) noexcept;

  public:
  auto operator++() noexcept -> iterator&;
  auto operator++(int) noexcept -> iterator;
  auto operator--() noexcept -> iterator&;
  auto operator--(int) noexcept -> iterator;

  auto get() const noexcept -> T*;
  auto operator*() const noexcept -> T&;
  auto operator->() const noexcept -> T*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  basic_linked_list::iterator iter_;
};


template<typename T>
class linked_list<T>::const_iterator {
  friend linked_list;
  friend iterator;

  public:
  using iterator_category = std::iterator_traits<basic_linked_list::const_iterator>::iterator_category;
  using value_type = T;
  using reference = const T&;
  using pointer = const T*;
  using difference_type = std::iterator_traits<basic_linked_list::const_iterator>::difference_type;

  const_iterator() noexcept;
  const_iterator(const iterator& y) noexcept;

  private:
  explicit const_iterator(basic_linked_list::const_iterator iter) noexcept;

  public:
  auto operator++() noexcept -> const_iterator&;
  auto operator++(int) noexcept -> const_iterator;
  auto operator--() noexcept -> const_iterator&;
  auto operator--(int) noexcept -> const_iterator;

  auto get() const noexcept -> const T*;
  auto operator*() const noexcept -> const T&;
  auto operator->() const noexcept -> const T*;

  auto operator==(const iterator& y) const noexcept -> bool;
  auto operator!=(const iterator& y) const noexcept -> bool;
  auto operator==(const const_iterator& y) const noexcept -> bool;
  auto operator!=(const const_iterator& y) const noexcept -> bool;

  private:
  basic_linked_list::const_iterator iter_;
};


} /* namespace libhoard::detail */

#include "linked_list.ii"
