#ifndef _RADIANT_OM_REGION_H
#define _RADIANT_OM_REGION_H

#include "dm/boxed.h"
#include "csg/region.h"
#include "radiant_macros.h"
#include "object_enums.h"

BEGIN_RADIANT_OM_NAMESPACE

// A region in a box!  region in boxes are useful for registering
// callbacks on regions when they change
typedef dm::Boxed<csg::Region2, Region2BoxedObjectType> Region2Boxed;
typedef dm::Boxed<csg::Region3, Region3BoxedObjectType> Region3Boxed;

// A pointer to a region in a box...  useful for sharing regions
// between multiple parties (e.g. your collision shape and your
// area of interest in a destination)
DECLARE_SHARED_POINTER_TYPES(Region2Boxed);
DECLARE_SHARED_POINTER_TYPES(Region3Boxed);

// A poitner to a region in a box, in a box!  useful to checking
// when pointers that are shared with other people change.
typedef dm::Boxed<Region2BoxedPtr> Region2BoxedPtrBoxed;
typedef dm::Boxed<Region3BoxedPtr> Region3BoxedPtrBoxed;

// Now you want to know if any thing all they way donw the chain changes.
// This can happen either because:
//   1) A user allocates a new region ptr and hands it to you (the
//      Region3BoxedPtrBoxed changes).
//   2) A region you're already pointing to changes (the region in the
//      Region3Boxed changes).
//
// That's what the DeepRegionGuard is for!

struct DeepRegionGuard {
   core::Guard   region;        // trace the region in the Region3Boxed
   core::Guard   boxed;         // trace the pointer in the Region3BoxedPtrBoxed
   dm::ObjectId store_id;
   dm::ObjectId object_id;

   DeepRegionGuard(dm::ObjectId s, dm::ObjectId o) : 
      store_id(s),
      object_id(o)
   {
   }

   ~DeepRegionGuard() {
   }
};

DECLARE_SHARED_POINTER_TYPES(DeepRegionGuard)

dm::GenerationId DeepObj_GetLastModified(Region3BoxedPtrBoxed const& boxedRegionPtrField);

DeepRegionGuardPtr DeepTraceRegion(Region3BoxedPtrBoxed const& boxedRegionPtrField,
                                   const char* reason,
                                   std::function<void(csg::Region3 const& r)> updateCb);

DeepRegionGuardPtr DeepTraceRegionVoid(Region3BoxedPtrBoxed const& boxedRegionPtrField,
                                       const char* reason,
                                       std::function<void()> updateCb);


class Region3BoxedPromise
{
public:
   Region3BoxedPromise(Region3BoxedPtrBoxed const& boxedRegionPtrField, const char* reason);

public:
   Region3BoxedPromise* PushChangedCb(luabind::object cb);

private:
   DeepRegionGuardPtr           region_guard_;
   std::vector<luabind::object>  changedCbs_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
