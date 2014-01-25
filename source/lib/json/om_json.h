#ifndef _RADIANT_LIB_JSON_OM_OM_JSON_H
#define _RADIANT_LIB_JSON_OM_OM_JSON_H

#include "om/namespace.h"
#include "om/region.h"
#include "om/om.h"
#include "om/lua/lua_om.h" // xxx: uggg.
#include "lib/json/dm_json.h"

BEGIN_RADIANT_JSON_NAMESPACE

DECLARE_JSON_CODEC_TEMPLATES(om::JsonBoxed)
DECLARE_JSON_CODEC_TEMPLATES(om::Region3BoxedPtrBoxed)

#define OM_OBJECT(Cls, lower) DECLARE_JSON_CODEC_TEMPLATES(om::Cls)
OM_ALL_OBJECTS
OM_ALL_COMPONENTS
#undef OM_OBJECT

END_RADIANT_JSON_NAMESPACE

#endif // _RADIANT_LIB_JSON_OM_OM_JSON_H
