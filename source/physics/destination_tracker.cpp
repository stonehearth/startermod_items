#include "radiant.h"
#include "dm/boxed_trace.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "destination_tracker.h"
#include "om/components/destination.ridl.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * DestinationTracker::DestinationTracker
 *
 * Track the shape of the region of an entity for the NavGrid.
 *
 */

DestinationTracker::DestinationTracker(NavGrid& ng, om::EntityPtr entity, om::DestinationPtr dst) :
   RegionTracker(ng, NON_COLLISION, entity),
   dst_(dst)
{
}

DestinationTracker::~DestinationTracker()
{
}

/*
 * DestinationTracker::Initialize
 *
 * Put a trace on the region to notify the NavGrid whenever it changes.
 *
 */

void DestinationTracker::Initialize()
{
   RegionTracker::Initialize();

   om::DestinationPtr dst = dst_.lock();
   if (dst) {
      SetRegionTrace(dst->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * DestinationTracker::GetRegion
 *
 * Helper method to get the region from the encapsulated instance.
 */

om::Region3BoxedPtr DestinationTracker::GetRegion() const
{
   om::DestinationPtr dst = dst_.lock();
   if (dst) {
      return dst->GetRegion();
   }
   return nullptr;
}
