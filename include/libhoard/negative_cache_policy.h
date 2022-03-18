#pragma once

namespace libhoard {


/**
 * Policy that allows error state to be kept in the cache.
 *
 * Should be paired with one or more policies that deal with when an error
 * should expire.
 */
class negative_cache_policy {
  private:
  class value_base_impl {
    protected:
    template<typename Table>
    explicit value_base_impl([[maybe_unused]] const Table& table) noexcept {}

    ~value_base_impl() noexcept = default;
  };

  public:
  template<typename Table>
  using value_base = value_base_impl;
};


} /* namespace libhoard */
