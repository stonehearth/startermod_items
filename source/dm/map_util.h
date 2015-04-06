#ifndef _RADIANT_DM_MAP_UTIL_H_
#define _RADIANT_DM_MAP_UTIL_H_

#include "dm.h"
#include <unordered_set>
#include <tbb/spin_mutex.h>

BEGIN_RADIANT_DM_NAMESPACE

/*
 * class NopKeyTransform
 *
 * The default KeyTransform functor for dm::Maps.  Does nothing.
 */

template <typename K>
struct NopKeyTransform {
   K const& operator()(K const& key) {
      return key;
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_UTIL_H_
