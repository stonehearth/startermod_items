#include "radiant.h"
#include "om/components/region_collision_shape.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "region_collision_shape_tracker.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * RegionCollisionShapeTracker::RegionCollisionShapeTracker
 *
 * Track the RegionCollisionShape for an Entity.
 */
RegionCollisionShapeTracker::RegionCollisionShapeTracker(NavGrid& ng, om::EntityPtr entity, om::RegionCollisionShapePtr rcs) :
   RegionTracker(ng, entity),
   rcs_(rcs)
{
}

/*
 * RegionCollisionShapeTracker::Initialize
 *
 * Put a trace on the region for the RegionCollisionShape to notify the NavGrid
 * whenever the collision shape changes.
 */
void RegionCollisionShapeTracker::Initialize()
{
   CollisionTracker::Initialize();

   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) { 
      SetRegionTrace(rcs->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * RegionCollisionShapeTracker::GetRegion
 *
 * Helper method to get the current collision shape region.
 */
om::Region3BoxedPtr RegionCollisionShapeTracker::GetRegion() const
{
   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) {
      return rcs->GetRegion();
   }
   return nullptr;
}

TrackerType RegionCollisionShapeTracker::GetType() const
{
   return COLLISION;
}
