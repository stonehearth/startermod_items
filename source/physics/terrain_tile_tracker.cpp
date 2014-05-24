#include "radiant.h"
#include "om/region.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "terrain_tile_tracker.h"

using namespace radiant;
using namespace radiant::phys;

TerrainTileTracker::TerrainTileTracker(NavGrid& ng, om::EntityPtr entity, csg::Point3 const& offset, om::Region3BoxedPtr region) :
   CollisionTracker(ng, entity),
   offset_(offset),
   region_(region),
   last_bounds_(csg::Cube3::zero)
{
}

void TerrainTileTracker::Initialize()
{
   CollisionTracker::Initialize();

   om::Region3BoxedPtr region = region_.lock();
   if (region) {
      trace_ = region->TraceChanges("nav grid", GetNavGrid().GetTraceCategory())
         ->OnModified([this]() {
            MarkChanged();
         })
         ->PushObjectState();
   }
}

void TerrainTileTracker::MarkChanged()
{
   om::Region3BoxedPtr region = region_.lock();
   if (region) {
      ASSERT(GetEntityPosition() == csg::Point3::zero);

      localRegion_ = region->Get().Translated(offset_);
      csg::Cube3 bounds = localRegion_.GetBounds();

      // xxx: we could just always pass in the bounding box of the terrain tile, but that's difficult
      // to complicate until the tile terrain branch gets pushed... (sigh)
      GetNavGrid().OnTrackerBoundsChanged(last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}

csg::Region3 TerrainTileTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3BoxedPtr region = region_.lock();
   if (region) {
      ASSERT(GetEntityPosition() == csg::Point3::zero);
      return localRegion_ & bounds;
   }
   return csg::Region3::empty;
}

TrackerType TerrainTileTracker::GetType() const
{
   return COLLISION;
}


/*
 * -- TerrainTileTracker::GetLocalRegion
 *
 * Return region tracked by this CollisionTracker in the coordinate system of the
 * owning entity.
 *
 * xxx: This is quite annoying!  GetLocalRegion returns a reference to some region
 * for performance purposes.  It expects this region to be in the coordinate system
 * of the Entity.  Unfortunately, our region is in the coordinate system of the 
 * _tile_.  So we have to copy the region to localRegion_ every time it changes,
 * and move it over by offset_.  That's expensive.   To fix this, store the
 * terrain tile region in the coordinate system of the *parent*.
 *
 */

csg::Region3 const& TerrainTileTracker::GetLocalRegion() const
{
   return localRegion_;
}

bool TerrainTileTracker::Intersects(csg::Cube3 const& worldBounds) const
{
   om::Region3BoxedPtr region = region_.lock();
   if (region) {
      return localRegion_.Intersects(worldBounds.Translated(-offset_));
   }
   return false;
}
