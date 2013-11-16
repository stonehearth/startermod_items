#ifndef _RADIANT_DM_PERSIST_PERSIST_H
#define _RADIANT_DM_PERSIST_PERSIST_H

#include "dm/dm.h"

BEGIN_RADIANT_DM_NAMESPACE

enum SaveLoadFlags {
   INCREMENTAL = (1 << 0)
};

template <typename T> void Save(T const&, protobuf::dm::Object &, int flags);
template <typename T> void Load(T&, const protobuf::dm::Object &, int flags);

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_PERSIST_PERSIST_H
