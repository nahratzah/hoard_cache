#pragma once

#include <memory>
#include <tuple>

namespace libhoard {


///\brief Policy that installs std::enable_shared_from_this onto the hashtable.
///\details This policy is intended to be used as a dependency for other policies.
class shared_from_this_policy {
  public:
  template<typename HashTable, typename ValueType, typename Allocator>
  class table_base
  : public std::enable_shared_from_this<HashTable>
  {
    public:
    template<typename... Args>
    table_base([[maybe_unused]] const std::tuple<Args...>& args, [[maybe_unused]] const Allocator& alloc) {}
  };
};


} /* namespace libhoard */
