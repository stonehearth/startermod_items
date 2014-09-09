#include "radiant.h"
#include "dm/boxed_trace.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "vertical_pathing_region_tracker.h"
#include "om/components/vertical_pathing_region.ridl.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * VerticalPathingRegionTracker::VerticalPathingRegionTracker
 *
 * Track the shape of the region of an entity for the NavGrid.
 *
 */

VerticalPathingRegionTracker::VerticalPathingRegionTracker(NavGrid& ng, om::EntityPtr entity, om::VerticalPathingRegionPtr vpr) :
   RegionTracker(ng, LADDER, entity),
   vpr_(vpr)
{
}

VerticalPathingRegionTracker::~VerticalPathingRegionTracker()
{
}

/*
 * VerticalPathingRegionTracker::Initialize
 *
 * Put a trace on the region to notify the NavGrid whenever it changes.
 *
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
 * Helper method to get the region from the encapsulated instance.
 */

om::Region3BoxedPtr VerticalPathingRegionTracker::GetRegion() const
{
   om::VerticalPathingRegionPtr vpr = vpr_.lock();
   if (vpr) {
      return vpr->GetRegion();
   }
   return nullptr;
}
