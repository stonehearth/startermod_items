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

#if 0
template <class T, int OT>
Node encode(class dm::Boxed<T, OT>::Promise const &obj) {
   throw std::invalid_argument(BUILD_STRING("cannot convert boxed " << GetShortTypeName<T>() << " promise to json"));
}

template <class K, class V, class Hash>
Node encode(class dm::Map<K, V, Hash>::LuaIterator const &obj) {
   throw std::invalid_argument(BUILD_STRING("cannot convert map iterator to json"));
}

template <class K, class V, class Hash>
Node encode(class dm::Map<K, V, Hash>::LuaPromise const &obj) {
   throw std::invalid_argument(BUILD_STRING("cannot convert map promise to json"));
}
#endif

END_RADIANT_JSON_NAMESPACE

#endif // _RADIANT_LIB_JSON_DM_DM_JSON_H
