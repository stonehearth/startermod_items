#ifndef _RADIANT_OM_REGION_H
#define _RADIANT_OM_REGION_H

#include "dm/boxed.h"
#include "csg/region.h"
#include "radiant_macros.h"
#include "object_enums.h"

BEGIN_RADIANT_OM_NAMESPACE

// xxx - rename BoxedRegion3 to somethign more relevant!
typedef dm::Boxed<csg::Region3, BoxedRegion3ObjectType> BoxedRegion3;

DECLARE_SHARED_POINTER_TYPES(BoxedRegion3);

struct BoxedRegionGuard {
   dm::Guard   region;
   dm::Guard   boxed;
   dm::ObjectId store_id;
   dm::ObjectId object_id;

   BoxedRegionGuard(dm::ObjectId s, dm::ObjectId o) : 
      store_id(s),
      object_id(o)
   {
   }

   ~BoxedRegionGuard() {
      LOG(WARNING) << "(xyz) killing boxed region guard for " << store_id << ":" << object_id;
   }
};

DECLARE_SHARED_POINTER_TYPES(BoxedRegionGuard)


BoxedRegionGuardPtr TraceBoxedRegion3PtrField(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField,
                                              const char* reason,
                                              std::function<void(csg::Region3 const& r)> updateCb);

BoxedRegionGuardPtr TraceBoxedRegion3PtrFieldVoid(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField,
                                                  const char* reason,
                                                  std::function<void()> updateCb);


class BoxedRegion3Promise
{
public:
   BoxedRegion3Promise(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField, const char* reason);

public:
   BoxedRegion3Promise* PushChangedCb(luabind::object cb);

private:
   BoxedRegionGuardPtr           region_guard_;
   std::vector<luabind::object>  changedCbs_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
