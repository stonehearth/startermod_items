#ifndef _RADIANT_OM_ALLOC_H
#define _RADIANT_OM_ALLOC_H

#include <memory>
#include "om.h"
#include "dm/dm.h"
#include "dm/object.h"

BEGIN_RADIANT_OM_NAMESPACE

std::shared_ptr<dm::Object> AllocObject(dm::ObjectType type, dm::Store& store);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
