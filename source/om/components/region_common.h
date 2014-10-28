#ifndef _RADIANT_OM_REGION_COMMON_H
#define _RADIANT_OM_REGION_COMMON_H

#include "libjson.h"
#include "dm/boxed.h"
#include "dm/store.h"
#include "om/region.h"
#include "lib/json/json.h"

BEGIN_RADIANT_OM_NAMESPACE

Region3fBoxedPtr LoadRegion(radiant::json::Node const& obj, radiant::dm::Store& store);

END_RADIANT_OM_NAMESPACE

#endif