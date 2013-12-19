#include "radiant.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "vertical_pathing_region_tracker.h"

using namespace radiant;
using namespace radiant::phys;

VerticalPathingRegionTracker::VerticalPathingRegionTracker(NavGrid& ng, om::EntityPtr entity, om::VerticalPathingRegionPtr vpr) :
   CollisionTracker(ng, entity),
   vpr_(vpr),
   last_bounds_(csg::Cube3::zero)
{
}

void VerticalPathingRegionTracker::Initialize()
{
   CollisionTracker::Initialize();

   om::VerticalPathingRegionPtr vpr = vpr_.lock();
   if (vpr) {
      trace_ = vpr->TraceRegion("nav grid", GetNavGrid().GetTraceCategory())
         ->OnModified([this]() {
            MarkChanged();
         })
         ->PushObjectState();
   }
}

void VerticalPathingRegionTracker::MarkChanged()
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      csg::Cube3 bounds = region->Get().GetBounds();
      bounds.Translate(GetEntityPosition());
      GetNavGrid().AddCollisionTracker(NavGridTile::LADDER, last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}

csg::Region3 VerticalPathingRegionTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      return region->Get().Translated(GetEntityPosition()) & bounds;
   }
   return csg::Region3::empty;
}

om::Region3BoxedPtr VerticalPathingRegionTracker::GetRegion() const
{
   om::VerticalPathingRegionPtr vpr = vpr_.lock();
   if (vpr) {
      return vpr->GetRegion();
   }
   return nullptr;
}
