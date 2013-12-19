#include "radiant.h"
#include "om/components/region_collision_shape.ridl.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "region_collision_shape_tracker.h"

using namespace radiant;
using namespace radiant::phys;

RegionCollisionShapeTracker::RegionCollisionShapeTracker(NavGrid& ng, om::EntityPtr entity, om::RegionCollisionShapePtr rcs) :
   CollisionTracker(ng, entity),
   rcs_(rcs),
   last_bounds_(csg::Cube3::zero)
{
}

void RegionCollisionShapeTracker::Initialize()
{
   CollisionTracker::Initialize();

   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) {
      trace_ = rcs->TraceRegion("nav grid", GetNavGrid().GetTraceCategory())
         ->OnModified([this]() {
            MarkChanged();
         })
         ->PushObjectState();
   }
}

void RegionCollisionShapeTracker::MarkChanged()
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      csg::Cube3 bounds = region->Get().GetBounds();
      bounds.Translate(GetEntityPosition());
      GetNavGrid().AddCollisionTracker(NavGridTile::COLLISION, last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}

csg::Region3 RegionCollisionShapeTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      return region->Get().Translated(GetEntityPosition()) & bounds;
   }
   return csg::Region3::empty;
}

om::Region3BoxedPtr RegionCollisionShapeTracker::GetRegion() const
{
   om::RegionCollisionShapePtr rcs = rcs_.lock();
   if (rcs) {
      return rcs->GetRegion();
   }
   return nullptr;
}
