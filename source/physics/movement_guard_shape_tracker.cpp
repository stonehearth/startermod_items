#include "radiant.h"
#include "dm/boxed_trace.h"
#include "csg/point.h"
#include "csg/util.h"
#include "nav_grid.h"
#include "movement_guard_shape_tracker.h"
#include "om/components/movement_guard_shape.ridl.h"
#include "physics_util.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * MovementGuardShapeTracker::MovementGuardShapeTracker
 *
 * Track the shape of the region of an entity for the NavGrid.
 *
 */

MovementGuardShapeTracker::MovementGuardShapeTracker(NavGrid& ng, om::EntityPtr entity, om::MovementGuardShapePtr mgs) :
   RegionTracker(ng, TrackerType::MOVEMENT_GUARD, entity),
   mgs_(mgs)
{
}

MovementGuardShapeTracker::~MovementGuardShapeTracker()
{
}

/*
 * RegionCollisionShapeTracker::Initialize
 *
 * Put a trace on the region to notify the NavGrid whenever it changes.
 *
 */

void MovementGuardShapeTracker::Initialize()
{
   RegionTracker::Initialize();

   om::MovementGuardShapePtr mgs = mgs_.lock();
   if (mgs) {
      SetRegionTrace(mgs->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * RegionCollisionShapeTracker::GetRegion
 *
 * Helper method to get the region from the encapsulated instance.
 */

om::Region3fBoxedPtr MovementGuardShapeTracker::GetRegion() const
{
   om::MovementGuardShapePtr mgs = mgs_.lock();
   if (mgs) {
      return mgs->GetRegion();
   }
   return nullptr;
}

bool MovementGuardShapeTracker::CanPassThrough(om::EntityPtr const& entity, csg::Point3 const& location)
{
   om::MovementGuardShapePtr mgs = mgs_.lock();
   if (!mgs) {
      return false;
   }
   return mgs->CanPassThrough(entity, location);
}
