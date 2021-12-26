#pragma once

#include <cstddef>
#include <forward_list>
#include <optional>
#include <vector>

namespace libhoard {


template<typename ValueType, typename Allocator>
class hash_set_ {
  public:
  using value_type = ValueType;
  using key_type = typename value_type::key_type;
  using mapped_type = typename value_type::mapped_type;
  using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;

  private:
  using list_type = std::forward_list<value_type, allocator_type>;
  using hash_list_type = std::vector<typename list_type::iterator_type, typename std::allocator_traits<allocator_type>::template rebind_alloc<typename list_type::iterator_type>>;

  public:
  explicit hash_set_(allocator_type alloc = allocator_type()) : elems_(alloc), hash_(std::move(alloc)) {}

  template<typename EqFn>
  auto get_if_present(std::size_t hash, const EqFn& eqfn) const -> std::optional<mapped_type> {
    const auto br = before_range_(hash);
    const auto found = search_(br, eqfn);
    if (found == br.second) return std::nullopt;
    return std::make_optional(std::next(found)->mapped());
  }

  private:
  auto before_range_(std::size_t hash) const noexcept -> std::pair<typename list_type::const_iterator, const typename list_type::const_iterator&> {
    if (hash_.empty()) return std::make_pair(elems_.before_begin(), elems_.before_begin());
    const hash_list_type::size_type bucket_idx = hash % hash_.size();
    return std::make_pair(bucket_idx == 0 ? elems_.before_begin() : hash_[bucket_idx - 1u], hash_[bucket_idx]);
  }

  auto before_range_(std::size_t hash) noexcept -> std::pair<typename list_type::iterator, typename list_type::iterator&> {
    if (hash_.empty()) return std::make_pair(elems_.before_begin(), elems_.before_begin());
    const hash_list_type::size_type bucket_idx = hash % hash_.size();
    return std::make_pair(bucket_idx == 0 ? elems_.before_begin() : hash_[bucket_idx - 1u], hash_[bucket_idx]);
  }

  template<typename Iter>
  auto search_(std::pair<Iter, Iter> before_range, const EqFn& eqfn) const -> Iter {
    while (before_range.first != before_range.second && !eqfn(*std::next(before_range.first)))
      ++before_range.first;
    return before_range.first;
  }

  list_type elems_;
  hash_list_type hash_;
};


template<typename ValueType, typename Allocator, typename... Args>
class impl {
  public:
  using key_type = typename ValueType::key_type;
  using mapped_type = typename ValueType::mapped_type;
  using allocator_type = Allocator;

  private:
  hash_set_<ValueType, allocator_type> set_;
};


} /* namespace libhoard */
