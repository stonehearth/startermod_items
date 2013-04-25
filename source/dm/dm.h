#ifndef _RADIANT_DM_DM_H
#define _RADIANT_DM_DM_H

#include "namespace.h"
#include "guard_set.h"
#include "store.pb.h"
#include "types.h"
#include "dm_save_impl.h"

BEGIN_RADIANT_DM_NAMESPACE

class Object;
typedef std::weak_ptr<Object> ObjectRef;
typedef std::shared_ptr<Object> ObjectPtr;

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_DM_H
