#include "deferred.h"

using namespace radiant;
using namespace radiant::core;

std::atomic<DeferredBase::DeferredId> DeferredBase::next_deferred_id_(1);