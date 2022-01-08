#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>

#include "shared_from_this_policy.h"
#include "detail/meta.h"
#include "detail/refcount.h"

namespace libhoard {


template<typename Functor>
class resolver_policy {
  public:
  template<typename HashTable, typename ValueType, typename Allocator>
  class table_base {
    public:
    template<typename... Args>
    table_base(const std::tuple<Args...>& args, const Allocator& alloc)
    : resolver_(std::get<resolver_policy>(args).resolver_)
    {}

    template<typename... Keys>
    auto resolve(std::size_t hash, const Keys&... keys) -> detail::refcount_ptr<ValueType, Allocator> {
      return static_cast<HashTable*>(this)->allocate_value_type(std::piecewise_construct, std::forward_as_tuple(keys...), std::invoke(resolver_, keys...));
    }

    private:
    const Functor resolver_;
  };

  resolver_policy() = default;

  explicit resolver_policy(Functor resolver)
  : resolver_(std::move(resolver))
  {}

  private:
  Functor resolver_;
};


template<typename Functor>
class async_resolver_policy {
  private:
  template<typename HashTable, typename ValuePointer>
  static auto on_assign_(HashTable& hashtable, ValuePointer value_pointer, bool value, bool assigned_via_callback) noexcept -> void {
    hashtable.on_assign_(value_pointer, value, assigned_via_callback);
  }

  public:
  using dependencies = detail::type_list<shared_from_this_policy>;

  template<typename HashTable, typename ValueType, typename Allocator>
  class table_base {
    public:
    template<typename... Args>
    table_base(const std::tuple<Args...>& args, const Allocator& alloc)
    : resolver_(std::get<async_resolver_policy>(args).resolver_)
    {}

    template<typename... Keys>
    auto async_resolve(std::size_t hash, const Keys&... keys) -> detail::refcount_ptr<ValueType, Allocator> {
      using callback_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<callback>;

      auto self = static_cast<HashTable*>(this);
      auto new_value = self->allocate_value_type(std::piecewise_construct, std::forward_as_tuple(keys...));

      const std::shared_ptr<callback> callback_ptr = std::allocate_shared<callback>(callback_allocator_type(self->get_allocator()), self->shared_from_this(), new_value);
      resolver_(callback_ptr, keys...);

      self->link(hash, new_value);
      callback_ptr->live = true;
      return new_value;
    }

    class callback {
      friend table_base;

      public:
      callback(const std::shared_ptr<HashTable>& self, detail::refcount_ptr<ValueType, Allocator> value)
      : weak_self(self),
        value(std::move(value))
      {}

      callback(const callback&) = delete;

      ~callback() {
        if (live) cancel();
      }

      template<typename... Args>
      auto assign(Args&&... args) -> bool {
        if (called) return false;

        if (auto self = weak_self.lock()) {
          std::lock_guard<HashTable> lck{ *self };
          value->assign(std::forward<Args>(args)...);
          on_assign_(*self, value.get(), true, true);
          called = true;
          return true;
        } else {
          if (auto pending = value->get_pending()) pending->cancel();
          called = true;
          return false;
        }
      }

      auto assign_error(typename ValueType::error_type ex) noexcept -> bool {
        if (called) return false;

        if (auto self = weak_self.lock()) {
          std::lock_guard<HashTable> lck{ *self };
          value->assign_error(std::move(ex));
          on_assign_(*self, value.get(), false, true);
          called = true;
          return true;
        } else {
          if (auto pending = value->get_pending()) pending->cancel();
          called = true;
          return false;
        }
      }

      auto cancel() noexcept -> bool {
        if (called) return false;

        if (auto self = weak_self.lock()) {
          std::lock_guard<HashTable> lck{ *self };
          value->cancel();
        } else if (auto pending = value->get_pending()) {
          pending->cancel();
        }
        called = true;
        return true;
      }

      private:
      const std::weak_ptr<HashTable> weak_self;
      detail::refcount_ptr<ValueType, Allocator> value;
      bool called = false;
      bool live = false;
    };

    const Functor resolver_;
  };

  explicit async_resolver_policy(Functor resolver)
  : resolver_(std::move(resolver))
  {}

  private:
  const Functor resolver_;
};


} /* namespace libhoard */
