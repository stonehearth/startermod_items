#include "radiant.h"
#include "water_tight_region_builder.h"
#include "collision_tracker.h"
#include "nav_grid.h"
#include "csg/util.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::phys;

#define WATER_TIGHT_REGION_LOG(level)    LOG(simulation.water_tight_region, level)

WaterTightRegionBuilder::WaterTightRegionBuilder(NavGrid const& ng) :
   _navgrid(ng),
   _deltaAccumulator(nullptr)
{
   _onTileDirtyGuard = _navgrid.NotifyTileDirty([this](csg::Point3 const& pt) {
         OnTileDirty(pt);
      });
}

WaterTightRegionBuilder::~WaterTightRegionBuilder()
{
}

void WaterTightRegionBuilder::SetWaterTightRegion(om::Region3TiledPtr region,
                                                  dm::Boxed<csg::Region3f> *delta)
{
   _tiles = region;
   _deltaAccumulator = delta;
}

void WaterTightRegionBuilder::OnTileDirty(csg::Point3 const& addr)
{
   // Just remember that the tile is dirty for now.  We won't update the
   // water tight region until requested to by the simulation.
   _dirtyTiles.insert(addr);
}

void WaterTightRegionBuilder::UpdateRegion()
{
   int const BATCH_SIZE = 256;
   DirtyTileSet dirty = std::move(_dirtyTiles);
   csg::Region3 batch;

   for (csg::Point3 const& addr : dirty) {
      // Batch the dirty tiles together, but limit the batch size so we don't send
      // a gigantic region when the terrain changes a lot (like during world generation).
      csg::Region3 delta = GetTileDelta(addr);
      batch.Add(delta);

      if (batch.GetCubeCount() >= BATCH_SIZE) {
         _deltaAccumulator->Set(csg::ToFloat(batch));
         batch.Clear();
      }
   }

   if (!batch.IsEmpty()) {
      _deltaAccumulator->Set(csg::ToFloat(batch));
      batch.Clear();
   }
}

csg::Region3 WaterTightRegionBuilder::GetTileDelta(csg::Point3 const& index)
{
   csg::Point3 origin = index.Scaled(TILE_SIZE);
   csg::Cube3 bounds(origin, origin + csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));

   csg::Region3 current;

   // Build the water tight region of everything that overlaps this tile.
   _navgrid.GridTile(index).ForEachTracker([this, &current, &bounds](CollisionTrackerPtr tracker) mutable {
      TrackerType type = tracker->GetType();
      if (type == COLLISION || type == TERRAIN) {
         current.Add(tracker->GetOverlappingRegion(bounds));
      }
      return false;     // keep iterating
   });

   // The change is simply the difference of the two shapes.
   csg::Region3 delta;
   csg::Region3& last = *(_tiles->GetTile(index));
   if (last.IsEmpty()) {
      delta = current;
   } else if (current.IsEmpty()) {
      delta = last;
   } else {
      delta = (current - last) + (last - current);
   }
   last = current;   // Modifies that actual tile data in _tiles

   return delta;
}


/*
 * -- NavGrid::ShowDebugShapes
 *
 * Push some debugging information into the shaplist.
 */ 
void WaterTightRegionBuilder::ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg)
{
   csg::Point3 index = csg::GetChunkIndex<TILE_SIZE>(pt);
   csg::Region3 const& shape = *_tiles->GetTile(index);
   protocol::region3f* region = msg->add_region();

   csg::ToFloat(shape).SaveValue(region);
   csg::Color4(255, 0, 192, 64).SaveValue(region->mutable_color());
}