#pragma once

#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "detail/hashtable.h"
#include "detail/mapped_type.h"

namespace libhoard {


template<typename KeyType, typename T, typename... Policies>
class cache {
  private:
  using hashtable_type = detail::hashtable<KeyType, detail::mapped_value<T, std::allocator<T>, int>, Policies...>;

  public:
  template<typename... Args>
  explicit cache(Args&&... args) // XXX arguments
  : impl_(std::make_shared<hashtable_type>(std::forward_as_tuple(std::forward<Args>(args)...)))
  {}

  template<typename... Keys>
  auto get_if_exists(const Keys&... keys) const
      noexcept(noexcept(std::declval<hashtable_type&>().get_if_exists(std::declval<const Keys&>()...)))
  -> std::optional<T> {
    auto v = impl_->get_if_exists(keys...);
    switch (v.index()) {
      default:
        return std::nullopt;
      case 1:
        return std::make_optional(std::get<1>(std::move(v)));
    }
  }

  template<typename... Keys>
  auto get(const Keys&... keys) const
      noexcept(noexcept(std::declval<hashtable_type&>().get_if_exists(std::declval<const Keys&>()...)))
  -> std::optional<T> {
    auto v = impl_->get_if_exists(keys...);
    switch (v.index()) {
      default:
        return std::nullopt;
      case 1:
        return std::make_optional(std::get<1>(std::move(v)));
    }
  }

  auto clear() noexcept -> void {
    impl_->expire_all();
  }

  template<typename... Keys>
  auto erase(const Keys&... keys) noexcept -> void {
    impl_->expire(keys...);
  }

  template<typename... Keys, typename... MappedArgs>
  auto emplace(std::piecewise_construct_t pc, std::tuple<Keys...> keys, std::tuple<MappedArgs...> mapped) -> void {
    impl_->emplace(std::move(pc), std::move(keys), std::move(mapped));
  }

  template<typename KeyArg, typename MappedArg>
  auto emplace(KeyArg&& key, MappedArg&& mapped) -> void {
    impl_->emplace(std::forward<KeyArg>(key), std::forward<MappedArg>(mapped));
  }

  private:
  std::shared_ptr<hashtable_type> impl_;
};


} /* namespace libhoard */
