#pragma once

namespace libhoard {


/**
 * \brief Policy that controls the allocator for the hashtable.
 * \tparam Allocator An allocator. The element type of the allocator doesn't matter.
 * \ingroup libhoard_api
 */
template<typename Allocator>
class allocator {
  public:
  using allocator_type = Allocator;
};


} /* namespace libhoard */
