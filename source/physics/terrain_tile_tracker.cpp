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
      csg::Cube3 bounds = region->Get().GetBounds();
      bounds.Translate(offset_ + GetEntityPosition());

      // xxx: we could just always pass in the bounding box of the terrain tile, but that's difficult
      // to complicate until the tile terrain branch gets pushed... (sigh)
      GetNavGrid().AddCollisionTracker(last_bounds_, bounds, shared_from_this());
      last_bounds_ = bounds;
   }
}

csg::Region3 TerrainTileTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   om::Region3BoxedPtr region = region_.lock();
   if (region) {
      return region->Get().Translated(GetEntityPosition() + offset_) & bounds;
   }
   return csg::Region3::empty;
}

TrackerType TerrainTileTracker::GetType() const
{
   return COLLISION;
}

bool TerrainTileTracker::Intersects(csg::Cube3 const& worldBounds) const
{
   om::Region3BoxedPtr region = region_.lock();
   if (region) {
      csg::Point3 origin = GetEntityPosition();
      return region->Get().Intersects(worldBounds.Translated(-origin));
   }
   return false;
}
