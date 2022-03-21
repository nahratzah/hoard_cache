#pragma once

#include <tuple>

namespace libhoard::detail {


/**
 * \brief Policy that implements refresh logic.
 * \details
 * This is a policy that's used as a dependency by other policies.
 * It installs a `void HashTable::refresh(ValueType*)` function, which refreshes the value.
 * It is safe to call this function multiple times.
 *
 * In order for this policy to work, the cache must have a resolver policy.
 */
class refresh_impl_policy {
  public:
  class value_base;
  template<typename HashTable, typename ValueType, typename Allocator> class table_base;

  private:
  template<typename ValueType>
  static auto on_refresh_event(ValueType* new_value, const ValueType* old_value);
};

class refresh_impl_policy::value_base {
  template<typename HashTable, typename ValueType, typename Allocator> friend class refresh_impl_policy::table_base;

  public:
  template<typename HashTable>
  value_base(const HashTable& table);

  private:
  bool refresh_started_ = false;
};

template<typename HashTable, typename ValueType, typename Allocator>
class refresh_impl_policy::table_base {
  public:
  table_base(const refresh_impl_policy& policy, const Allocator& allocator);

  auto refresh(ValueType* raw_value_ptr) -> void;
};


} /* namespace libhoard::detail */

#include "refresh_impl_policy.ii"
