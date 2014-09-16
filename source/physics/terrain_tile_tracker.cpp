#include "radiant.h"
#include "om/region.h"
#include "csg/point.h"
#include "csg/util.h"
#include "nav_grid.h"
#include "dm/boxed_trace.h"
#include "terrain_tile_tracker.h"

using namespace radiant;
using namespace radiant::phys;

TerrainTileTracker::TerrainTileTracker(NavGrid& ng, om::EntityPtr entity, csg::Point3f const& offset, om::Region3fBoxedPtr region) :
   CollisionTracker(ng, TERRAIN, entity),
   offset_(offset),
   region_(region),
   last_bounds_(csg::CollisionBox::zero)
{
}

void TerrainTileTracker::Initialize()
{
   CollisionTracker::Initialize();

   om::Region3fBoxedPtr region = region_.lock();
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
   om::Region3fBoxedPtr region = region_.lock();
   if (region) {
      shape_ = region->Get().Translated(offset_);
      csg::CollisionBox bounds = shape_.GetBounds();

      // xxx: we could just always pass in the bounding box of the terrain tile, but that's difficult
      // to complicate until the tile terrain branch gets pushed... (sigh)
      GetNavGrid().OnTrackerBoundsChanged(last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}

csg::Region3 TerrainTileTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3fBoxedPtr region = region_.lock();
   if (region) {
      // xxx: if the terrain tile granularity and offset were identical to the nav grid tile, 
      // we could avoid this (potentially very expensive) math entirely and just return the shape!
      return csg::ToInt(shape_) & bounds;
   }
   return csg::Region3::empty;
}

bool TerrainTileTracker::Intersects(csg::CollisionBox const& worldBounds) const
{
   om::Region3fBoxedPtr region = region_.lock();
   if (region) {
      bool collision = shape_.Intersects(worldBounds);

      LOG(physics.navgrid, 8) << "checking collision of tile bounds " << shape_.GetBounds() << " against " << worldBounds << "(collide? " << std::boolalpha << collision << ")";
      return collision;
   }
   return false;
}
