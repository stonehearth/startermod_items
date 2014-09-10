#include "radiant.h"
#include "dm/boxed_trace.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "region_collision_shape_tracker.h"
#include "om/components/region_collision_shape.ridl.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * RegionCollisionShapeTracker::RegionCollisionShapeTracker
 *
 * Track the shape of the region of an entity for the NavGrid.
 *
 */

RegionCollisionShapeTracker::RegionCollisionShapeTracker(NavGrid& ng, TrackerType t, om::EntityPtr entity, om::RegionCollisionShapePtr rcs) :
   RegionTracker(ng, t, entity),
   rcs_(rcs)
{
}

RegionCollisionShapeTracker::~RegionCollisionShapeTracker()
{
}

/*
 * RegionCollisionShapeTracker::Initialize
 *
 * Put a trace on the region to notify the NavGrid whenever it changes.
 *
 */

void RegionCollisionShapeTracker::Initialize()
{
   RegionTracker::Initialize();

   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) {
      SetRegionTrace(rcs->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * RegionCollisionShapeTracker::GetRegion
 *
 * Helper method to get the region from the encapsulated instance.
 */

om::Region3fBoxedPtr RegionCollisionShapeTracker::GetRegion() const
{
   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) {
      return rcs->GetRegion();
   }
   return nullptr;
}
