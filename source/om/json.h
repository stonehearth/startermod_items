#ifndef _RADIANT_OM_JSON_H
#define _RADIANT_OM_JSON_H

#include "dm/dm.h"
#include "dm/boxed.h"
#include "dm/boxed_trace.h"
#include "lib/json/node.h"
#include "object_enums.h"

BEGIN_RADIANT_OM_NAMESPACE

// xxx: this is totally lame.  the data model should be able to convert things
// to json on it's on (via virtual dispatch) and we should just get rid of the
// om::ObjectFormatter.  That's a huge, huge change, though, so for now just
// wrap json nodes into the object model so they can be propertly traced by
// the chrome ui.
typedef dm::Boxed<json::Node, JsonBoxedObjectType> JsonBoxed;

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_JSON_H
