#ifndef _RADIANT_OM_PCH_H
#define _RADIANT_OM_PCH_H

#include "radiant.h"
#include "radiant_stdutil.h"
#include "radiant_luabind.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "dm/map.h"
#include "dm/store.h"
#include "om/om.h"

#define DEFINE_ALL_OBJECTS
#include "all_objects.h"
#include "all_components.h"

#endif // _RADIANT_OM_PCH_H