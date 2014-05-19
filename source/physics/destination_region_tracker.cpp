#include "radiant.h"
#include "om/components/destination.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "destination_region_tracker.h"

using namespace radiant;
using namespace radiant::phys;

/* 
 * -- DestinationRegionTracker::DestinationRegionTracker
 *
 * Construct a new DestinationRegionTracker.
 */
DestinationRegionTracker::DestinationRegionTracker(NavGrid& ng, om::EntityPtr entity, om::DestinationPtr dst) :
   RegionTracker(ng, entity),
   dst_(dst)
{
}

/* 
 * -- DestinationRegionTracker::Initialize
 *
 * Put a trace on the DestinationRegion to notify the NavGrid whenever it's shape changes.
 */
void DestinationRegionTracker::Initialize()
{
   RegionTracker::Initialize();

   om::DestinationPtr dst = dst_.lock();
   if (dst) {
      SetRegionTrace(dst->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * DestinationRegionTracker::GetRegion
 *
 * Helper method to get the current collision shape region.
 */
om::Region3BoxedPtr DestinationRegionTracker::GetRegion() const
{
   om::DestinationPtr dst = dst_.lock();
   if (dst) {
      return dst->GetRegion();
   }
   return nullptr;
}

TrackerType DestinationRegionTracker::GetType() const
{
   return DESTINATION;
}
