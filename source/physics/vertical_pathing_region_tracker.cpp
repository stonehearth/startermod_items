#include "radiant.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "vertical_pathing_region_tracker.h"

using namespace radiant;
using namespace radiant::phys;

   
/* 
 * -- VerticalPathingRegionTracker::VerticalPathingRegionTracker
 *
 * Construct a new VerticalPathingRegionTracker.
 */
VerticalPathingRegionTracker::VerticalPathingRegionTracker(NavGrid& ng, om::EntityPtr entity, om::VerticalPathingRegionPtr vpr) :
   RegionTracker(ng, entity),
   vpr_(vpr)
{
}

/* 
 * -- VerticalPathingRegionTracker::Initialize
 *
 * Put a trace on the VerticalPathingRegion to notify the NavGrid whenever it's shape changes.
 */
void VerticalPathingRegionTracker::Initialize()
{
   RegionTracker::Initialize();

   om::VerticalPathingRegionPtr vpr = vpr_.lock();
   if (vpr) {
      SetRegionTrace(vpr->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * VerticalPathingRegionTracker::GetRegion
 *
 * Helper method to get the current collision shape region.
 */
om::Region3BoxedPtr VerticalPathingRegionTracker::GetRegion() const
{
   om::VerticalPathingRegionPtr vpr = vpr_.lock();
   if (vpr) {
      return vpr->GetRegion();
   }
   return nullptr;
}

TrackerType VerticalPathingRegionTracker::GetType() const
{
   return LADDER;
}
