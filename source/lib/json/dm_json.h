#ifndef _RADIANT_LIB_JSON_DM_DM_JSON_H
#define _RADIANT_LIB_JSON_DM_DM_JSON_H

#include "dm/boxed.h"
#include "dm/map.h"
#include "lib/json/core_json.h"

BEGIN_RADIANT_JSON_NAMESPACE

template <class T, int OT>
Node encode(dm::Boxed<T, OT> const &obj) {
   return json::encode(obj.Get());
}

template <class K, class V, class Hash>
Node encode(dm::Map<K, V, Hash> const &obj) {
   Node result;
   for (auto const& entry : obj) {
      result.set(BUILD_STRING(entry.first), json::encode(entry.second));
   }
   return Node();
}

END_RADIANT_JSON_NAMESPACE

#endif // _RADIANT_LIB_JSON_DM_DM_JSON_H
