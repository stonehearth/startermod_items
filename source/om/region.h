#ifndef _RADIANT_OM_REGION_H
#define _RADIANT_OM_REGION_H

#include "dm/boxed.h"
#include "csg/region.h"
#include "object_enums.h"

BEGIN_RADIANT_OM_NAMESPACE

typedef dm::Boxed<csg::Region3, BoxedRegion3ObjectType> BoxedRegion3;
typedef std::shared_ptr<BoxedRegion3> BoxedRegion3Ptr;
typedef std::weak_ptr<BoxedRegion3> BoxedRegion3Ref;

dm::Guard TraceBoxedRegion3PtrField(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField,
                                    const char* reason,
                                    std::function<void(csg::Region3 const& r)> updateCb);

class BoxedRegion3Promise
{
public:
   BoxedRegion3Promise(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField, const char* reason);

public:
   BoxedRegion3Promise* PushChangedCb(luabind::object cb);

private:
   dm::Guard                     guard_;
   std::vector<luabind::object>  changedCbs_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
