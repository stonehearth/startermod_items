#include "radiant.h"
#include "dm/boxed_trace.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "movement_modifier_shape_tracker.h"
#include "om/components/movement_modifier_shape.ridl.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * MovementModifierShapeTracker::MovementModifierShapeTracker
 *
 * Track the shape of the region of an entity for the NavGrid.
 *
 */

MovementModifierShapeTracker::MovementModifierShapeTracker(NavGrid& ng, om::EntityPtr entity, om::MovementModifierShapePtr mms) :
   RegionTracker(ng, TrackerType::COLLISION, entity),
   mms_(mms)
{
}

MovementModifierShapeTracker::~MovementModifierShapeTracker()
{
}

/*
 * RegionCollisionShapeTracker::Initialize
 *
 * Put a trace on the region to notify the NavGrid whenever it changes.
 *
 */

void MovementModifierShapeTracker::Initialize()
{
   RegionTracker::Initialize();

   om::MovementModifierShapePtr mms = mms_.lock();
   if (mms) {
      SetRegionTrace(mms->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * RegionCollisionShapeTracker::GetRegion
 *
 * Helper method to get the region from the encapsulated instance.
 */

om::Region3fBoxedPtr MovementModifierShapeTracker::GetRegion() const
{
   om::MovementModifierShapePtr mms = mms_.lock();
   if (mms) {
      return mms->GetRegion();
   }
   return nullptr;
}
