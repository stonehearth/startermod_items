#include "radiant.h"
#include "om/components/region_collision_shape.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "derived_region_tracker.h"

using namespace radiant;
using namespace radiant::phys;

/*
 * DerivedRegionTracker::DerivedRegionTracker
 *
 * Track the shape of the region of an entity for the NavGrid.
 */
template <class T, TrackerType OT>
DerivedRegionTracker<T, OT>::DerivedRegionTracker(NavGrid& ng, om::EntityPtr entity, std::shared_ptr<T> regionProviderPtr) :
   RegionTracker(ng, entity),
   regionProviderRef_(regionProviderPtr)
{
}

/*
 * DerivedRegionTracker::Initialize
 *
 * Put a trace on the region to notify the NavGrid whenever it changes.
 */
template <class T, TrackerType OT>
void DerivedRegionTracker<T, OT>::Initialize()
{
   RegionTracker::Initialize();

   std::shared_ptr<T> regionProviderPtr = regionProviderRef_.lock();
   if (regionProviderPtr) {
      SetRegionTrace(regionProviderPtr->TraceRegion("nav grid", GetNavGrid().GetTraceCategory()));
   }
}

/*
 * DerivedRegionTracker::GetRegion
 *
 * Helper method to get the region from the encapsulated instance.
 */
template <class T, TrackerType OT>
om::Region3BoxedPtr DerivedRegionTracker<T, OT>::GetRegion() const
{
   std::shared_ptr<T> regionProviderPtr = regionProviderRef_.lock();
   if (regionProviderPtr) {
      return regionProviderPtr->GetRegion();
   }
   return nullptr;
}

template <class T, TrackerType OT>
TrackerType DerivedRegionTracker<T, OT>::GetType() const
{
   return OT;
}

// define instantiations of this template
template RegionCollisionShapeTracker;
template RegionNonCollisionShapeTracker;
template DestinationRegionTracker;
template VerticalPathingRegionTracker;
