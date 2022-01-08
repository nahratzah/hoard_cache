#pragma once

#include "equal.h"
#include "hash.h"
#include "max_size_policy.h"
#include "shared_from_this_policy.h"
#include "thread_safe_policy.h"
#include "weaken_policy.h"
#include "resolver_policy.h"

#include "detail/meta.h"
#include "detail/queue.h"

namespace libhoard {


using detail::type_list;
using detail::queue_policy;


} /* namespace libhoard */
