#pragma once

#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace libhoard::detail {


///\brief Facet of cache that performs synchronous get.
///\tparam Impl The derived type of cache.
template<typename Impl, typename HashTableType>
class cache_get_impl {
  protected:
  cache_get_impl() noexcept = default;
  cache_get_impl(const cache_get_impl&) noexcept = default;
  cache_get_impl(cache_get_impl&&) noexcept = default;
  ~cache_get_impl() noexcept = default;
  auto operator=(const cache_get_impl&) noexcept -> cache_get_impl& = default;
  auto operator=(cache_get_impl&&) noexcept -> cache_get_impl& = default;

  public:
  template<typename... Keys>
  auto get(const Keys&... keys)
      noexcept(noexcept(std::declval<HashTableType&>().get(std::declval<const Keys&>()...)))
  -> std::conditional_t<
      HashTableType::uses_resolver::value,
      std::variant<std::monostate, typename HashTableType::mapped_type, typename HashTableType::error_type>,
      std::optional<typename HashTableType::mapped_type>> {
    Impl*const self = static_cast<Impl*>(this);
    std::lock_guard<HashTableType> lck{ *self->impl_ };

    auto v = self->impl_->get(keys...);
    if constexpr(HashTableType::uses_resolver::value) {
      return v;
    } else {
      switch (v.index()) {
        default:
          return std::nullopt;
        case 1:
          return std::make_optional(std::get<1>(std::move(v)));
      }
    }
  }
};


///\brief Facet of cache that does not perform a get.
class cache_omit_get_impl {
  protected:
  cache_omit_get_impl() noexcept = default;
  cache_omit_get_impl(const cache_omit_get_impl&) noexcept = default;
  cache_omit_get_impl(cache_omit_get_impl&&) noexcept = default;
  ~cache_omit_get_impl() noexcept = default;
  auto operator=(const cache_omit_get_impl&) noexcept -> cache_omit_get_impl& = default;
  auto operator=(cache_omit_get_impl&&) noexcept -> cache_omit_get_impl& = default;
};


template<typename Impl, typename HashTableImpl>
using cache_get = std::conditional_t<
    HashTableImpl::uses_async_resolver::value,
    cache_omit_get_impl,
    cache_get_impl<Impl, HashTableImpl>>;


} /* namespace libhoard::detail */
