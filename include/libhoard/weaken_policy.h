#pragma once

namespace libhoard {


/**
 * \brief Policy that makes the cache weaken elements.
 * \details
 * If this policy is added to the cache, elements will be weakened.
 * This means that instead of removing the item from the cache, its
 * pointer will be turned into a weak pointer.
 *
 * This policy has no effect if the cache doesn't hold smart pointers.
 *
 * \note This policy doesn't add base types, instead the queue code checks
 * for presence and modifies behaviour accordingly.
 */
class weaken_policy {};


} /* namespace libhoard */
