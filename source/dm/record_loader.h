#ifndef _RADIANT_DM_RECORD_LOADER_H
#define _RADIANT_DM_RECORD_LOADER_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

void LoadObject(Record& r, Protocol::Value const& msg);
void SaveObject(Record const& r, Protocol::Value* msg);
void SaveObjectDelta(Record const& record, Protocol::Value* msg);

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_LOADER_H

