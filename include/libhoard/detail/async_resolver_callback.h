#pragma once

#include <memory>
#include <utility>
#include <mutex>

#include "refcount.h"

namespace libhoard::detail {


template<typename HashTable, typename ValueType, typename Allocator>
class async_resolver_callback {
  public:
  async_resolver_callback(
      const std::shared_ptr<HashTable>& self,
      refcount_ptr<ValueType, Allocator> new_value)
  : weak_self(self),
    new_value(std::move(new_value))
  {}

  async_resolver_callback(const async_resolver_callback&) = delete;
  async_resolver_callback(async_resolver_callback&&) = default;

  ~async_resolver_callback() {
    if (new_value != nullptr) cancel();
  }

  template<typename... Args>
  auto assign(Args&&... args) -> bool {
    if (called) return false;

    if (auto self = weak_self.lock()) {
      std::lock_guard<HashTable> lck{ *self };
      new_value->assign(std::forward<Args>(args)...);
      self->on_assign_(new_value.get(), true, true);
      called = true;
      return true;
    } else {
      if (auto pending = new_value->get_pending()) pending->cancel();
      called = true;
      return false;
    }
  }

  auto assign_error(typename ValueType::error_type ex) noexcept -> bool {
    if (called) return false;

    if (auto self = weak_self.lock()) {
      std::lock_guard<HashTable> lck{ *self };
      new_value->assign_error(std::move(ex));
      self->on_assign_(new_value.get(), false, true);
      called = true;
      return true;
    } else {
      if (auto pending = new_value->get_pending()) pending->cancel();
      called = true;
      return false;
    }
  }

  auto cancel() noexcept -> bool {
    if (called) return false;

    if (auto self = weak_self.lock()) {
      std::lock_guard<HashTable> lck{ *self };
      new_value->cancel();
    } else {
      if (auto pending = new_value->get_pending()) pending->cancel();
    }
    called = true;
    return true;
  }

  private:
  std::weak_ptr<HashTable> weak_self;
  detail::refcount_ptr<ValueType, Allocator> new_value;
  bool called = false;
};


} /* namespace libhoard::detail */
