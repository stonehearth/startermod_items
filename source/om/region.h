#ifndef _RADIANT_OM_REGION_H
#define _RADIANT_OM_REGION_H

#include "dm/dm.h"
#include "dm/boxed.h"
#include "dm/boxed_trace.h"
#include "csg/region.h"
#include "core/guard.h"
#include "radiant_macros.h"
#include "object_enums.h"
#include "lib/lua/bind.h"

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

// A pointer to a region in a box, in a box!  useful to checking
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

struct DeepRegionGuard : public std::enable_shared_from_this<DeepRegionGuard> {
   dm::TracePtr   region_trace;
   std::shared_ptr<dm::BoxedTrace<Region3BoxedPtrBoxed>>   boxed_trace;
   dm::ObjectId   store_id;
   dm::ObjectId   object_id;
   std::function<void(csg::Region3 const&)> changed_cb_;

   DeepRegionGuard(dm::ObjectId s, dm::ObjectId o) : 
      store_id(s),
      object_id(o)
   {
   }

   virtual ~DeepRegionGuard() {
   }

   std::shared_ptr<DeepRegionGuard> OnChanged(std::function<void(csg::Region3 const& r)> const& cb)
   {
      changed_cb_ = cb;
      return shared_from_this();
   }

   std::shared_ptr<DeepRegionGuard> OnModified(std::function<void()> const& cb)
   {
      changed_cb_ = [cb](csg::Region3 const& r) {
         cb();
      };
      return shared_from_this();
   }

   std::shared_ptr<DeepRegionGuard> PushObjectState()
   {      
      boxed_trace->PushObjectState();
      return shared_from_this();
   }

   void SignalChanged(csg::Region3 const& r) {
      if (changed_cb_) {
         changed_cb_(r);
      }
   }
};

DECLARE_SHARED_POINTER_TYPES(DeepRegionGuard)

dm::GenerationId DeepObj_GetLastModified(Region3BoxedPtrBoxed const& boxedRegionPtrField);

DeepRegionGuardPtr DeepTraceRegion(Region3BoxedPtrBoxed const& boxedRegionPtrField,
                                   const char* reason, int category);



END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
