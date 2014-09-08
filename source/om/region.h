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
typedef dm::Boxed<csg::Region2,  Region2BoxedObjectType>  Region2Boxed;
typedef dm::Boxed<csg::Region3,  Region3BoxedObjectType>  Region3Boxed;
typedef dm::Boxed<csg::Region2f, Region2fBoxedObjectType> Region2fBoxed;
typedef dm::Boxed<csg::Region3f, Region3fBoxedObjectType> Region3fBoxed;

// A pointer to a region in a box...  useful for sharing regions
// between multiple parties (e.g. your collision shape and your
// area of interest in a destination)
DECLARE_SHARED_POINTER_TYPES(Region2Boxed);
DECLARE_SHARED_POINTER_TYPES(Region3Boxed);
DECLARE_SHARED_POINTER_TYPES(Region2fBoxed);
DECLARE_SHARED_POINTER_TYPES(Region3fBoxed);

// A pointer to a region in a box, in a box!  useful to checking
// when pointers that are shared with other people change.
typedef dm::Boxed<Region2BoxedPtr>  Region2BoxedPtrBoxed;
typedef dm::Boxed<Region3BoxedPtr>  Region3BoxedPtrBoxed;
typedef dm::Boxed<Region2fBoxedPtr> Region2fBoxedPtrBoxed;
typedef dm::Boxed<Region3fBoxedPtr> Region3fBoxedPtrBoxed;

// Now you want to know if any thing all they way donw the chain changes.
// This can happen either because:
//   1) A user allocates a new region ptr and hands it to you (the
//      Region3BoxedPtrBoxed changes).
//   2) A region you're already pointing to changes (the region in the
//      Region3Boxed changes).
//
// That's what the DeepRegionGuard is for!

template <typename OuterBox>
struct DeepRegionGuard : public std::enable_shared_from_this<DeepRegionGuard<OuterBox>> {
   typedef typename OuterBox::Value::element_type InnerBox;
   typedef typename InnerBox::Value Region;
   typedef std::shared_ptr<dm::BoxedTrace<OuterBox>> OuterBoxTracePtr;

   dm::TracePtr      region_trace;
   OuterBoxTracePtr  outer_box_trace;
   dm::ObjectId      store_id;
   dm::ObjectId      object_id;
   std::function<void(Region const&)> changed_cb_;

   DeepRegionGuard(dm::ObjectId s, dm::ObjectId o) : 
      store_id(s),
      object_id(o)
   {
   }

   virtual ~DeepRegionGuard() {
   }

   std::shared_ptr<DeepRegionGuard> OnChanged(std::function<void(Region const& r)> const& cb)
   {
      changed_cb_ = cb;
      return shared_from_this();
   }

   std::shared_ptr<DeepRegionGuard> OnModified(std::function<void()> const& cb)
   {
      changed_cb_ = [cb](Region const& r) {
         cb();
      };
      return shared_from_this();
   }

   std::shared_ptr<DeepRegionGuard> PushObjectState()
   {      
      outer_box_trace->PushObjectState();
      return shared_from_this();
   }

   void SignalChanged(Region const& r) {
      if (changed_cb_) {
         changed_cb_(r);
      }
   }
};

typedef DeepRegionGuard<Region2BoxedPtrBoxed>     DeepRegion2Guard;
typedef DeepRegionGuard<Region3BoxedPtrBoxed>     DeepRegion3Guard;
typedef DeepRegionGuard<Region2fBoxedPtrBoxed>    DeepRegion2fGuard;
typedef DeepRegionGuard<Region3fBoxedPtrBoxed>    DeepRegion3fGuard;

DECLARE_SHARED_POINTER_TYPES(DeepRegion2Guard)
DECLARE_SHARED_POINTER_TYPES(DeepRegion3Guard)
DECLARE_SHARED_POINTER_TYPES(DeepRegion2fGuard)
DECLARE_SHARED_POINTER_TYPES(DeepRegion3fGuard)

#define DRG_LOG(level)   LOG(om.region, level)

template <typename OuterBox>
std::shared_ptr<DeepRegionGuard<OuterBox>> DeepTraceRegion(OuterBox const& box,
                                                           const char* reason, int category)
{
   typedef DeepRegionGuard<OuterBox>::InnerBox InnerBox;
   std::shared_ptr<DeepRegionGuard<OuterBox>> result = std::make_shared<DeepRegionGuard<OuterBox>>(box.GetStoreId(),
                                                                                                   box.GetObjectId());
   std::weak_ptr<DeepRegionGuard<OuterBox>> r = result;

   auto outer_box_trace = box.TraceChanges(reason, category);
   result->outer_box_trace = outer_box_trace;

   DRG_LOG(5) << "tracing boxed region " << box.GetObjectId() << " (" << reason << ")";

   outer_box_trace->OnChanged([r, reason, category](OuterBox::Value const& value) {
         DRG_LOG(7) << "region pointer in box is now " << value;
         auto guard = r.lock();
         if (guard) {
            if (value) {
               DRG_LOG(7) << "installing new trace on " << value;
               auto region_trace = value->TraceChanges(reason, category);
               guard->region_trace = region_trace;

               region_trace
                  ->OnChanged([r](InnerBox::Value const& region) {
                     DRG_LOG(7) << "value of region pointer in box changed.  signaling change cb with " << region;
                     auto guard = r.lock();
                     if (guard) {
                        guard->SignalChanged(region);
                     }
                  })
                  ->PushObjectState();
            } else {
               DRG_LOG(7) << "signalling change cb with empty region ";
               guard->SignalChanged(InnerBox::Value());
               guard->region_trace = nullptr;
            }
         }
      })
      ->PushObjectState();

   return result;
}

#undef DRG_LOG

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
