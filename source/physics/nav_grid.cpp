#include "radiant.h"
#include "radiant_macros.h"
#include "nav_grid.h"
#include "nav_grid_tile.h"
#include "dm/store.h"
#include "csg/util.h"
#include "om/components/component.h"
#include "om/components/mob.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "om/components/region_collision_shape.ridl.h"
#include "terrain_tracker.h"
#include "terrain_tile_tracker.h"
#include "region_collision_shape_tracker.h"
#include "vertical_pathing_region_tracker.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG(physics.navgrid, level)

template <class T, typename C>
T ToLocalCoordinates(const T& coord, const std::shared_ptr<C> component)
{
   om::Entity const& entity = component->GetEntity(); 

   auto mob = entity.GetComponent<om::Mob>();
   if (mob) {
      T result(coord);
      csg::Point3 origin = mob->GetWorldGridLocation();
      result.Translate(-origin);
      return result;
   }
   return coord;
}

NavGrid::NavGrid(int trace_category) :
   trace_category_(trace_category)
{
}

void NavGrid::TrackComponent(std::shared_ptr<dm::Object> component)
{
   dm::ObjectId id = component->GetObjectId();

   switch (component->GetObjectType()) {
   case om::TerrainObjectType: {
      auto terrain = std::static_pointer_cast<om::Terrain>(component);
      CollisionTrackerPtr tracker = std::make_shared<TerrainTracker>(*this, terrain->GetEntityPtr(), terrain);
      object_collsion_trackers_[id] = tracker;
      tracker->Initialize();
      break;
   }
   case om::RegionCollisionShapeObjectType: {
      auto rcs = std::static_pointer_cast<om::RegionCollisionShape>(component);
      CollisionTrackerPtr tracker = std::make_shared<RegionCollisionShapeTracker>(*this, rcs->GetEntityPtr(), rcs);
      object_collsion_trackers_[id] = tracker;
      tracker->Initialize();
      break;
   }
   case om::VerticalPathingRegionObjectType: {
      auto rcs = std::static_pointer_cast<om::VerticalPathingRegion>(component);
      CollisionTrackerPtr tracker = std::make_shared<VerticalPathingRegionTracker>(*this, rcs->GetEntityPtr(), rcs);
      object_collsion_trackers_[id] = tracker;
      tracker->Initialize();
      break;
   }
   }
}

void NavGrid::AddTerrainTileTracker(om::EntityRef e, csg::Point3 const& offset, om::Region3BoxedPtr tile)
{
   NG_LOG(3) << "tracking terrain tile at " << offset;
   om::EntityPtr entity = e.lock();
   if (entity) {
      CollisionTrackerPtr tracker = std::make_shared<TerrainTileTracker>(*this, entity, offset, tile);
      terrain_tile_collsion_trackers_[offset] = tracker;
      tracker->Initialize();
   }
}

bool NavGrid::IsValidStandingRegion(csg::Region3 const& r) const
{
   for (csg::Cube3 const& cube : r) {
      if (!CanStandOn(cube)) {
         return false;
      }
   }
   return true;
}

// xxx: we need a "non-walkable" region to clip against.  until that exists,
// do the slow thing...
void NavGrid::RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& r) const
{
   csg::Region3 r2;
   for (csg::Cube3 const& cube : r) {
      for (csg::Point3 const& pt : cube) {
         if (CanStandOn(entity, pt)) {
            r2.AddUnique(pt);
         }
      }
   }
   r = r2;
}

bool NavGrid::IsEmpty(csg::Cube3 const& bounds) const
{
   bool success = csg::PartitionCubeIntoChunks(bounds, NavGridTile::TILE_SIZE, 
      [this](csg::Point3 const& index, csg::Cube3 const& cube) {
         bool abort = !GridTile(index).IsEmpty(cube);
         return !abort;
      });

   return success;
}

// xxx: this is horribly, horribly expensive!  we should do this
// with region clipping or somethign...
bool NavGrid::CanStandOn(csg::Cube3 const& cube) const
{
   csg::Point3 const& min = cube.GetMin();
   csg::Point3 const& max = cube.GetMax();
   csg::Point3 i;

   i.y = min.y;
   for (i.x = min.x; i.x < max.x; i.x++) {
      for(i.z = min.z; i.z < max.z; i.z++) {
         // xxx:  remove this function and pass a collision shape
         if (!CanStandOn(nullptr, i)) {
            return false;
         }
      }
   }
   return true;
}

bool NavGrid::CanStandOn(om::EntityPtr entity, csg::Point3 const& pt) const
{
   csg::Point3 index, offset;
   csg::GetChunkIndex(pt, NavGridTile::TILE_SIZE, index, offset);
   return GridTile(index).CanStandOn(offset);
}

int NavGrid::GetTraceCategory() const
{
   return trace_category_;
}

void NavGrid::AddCollisionTracker(NavGridTile::TrackerType type, csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker)
{
   NG_LOG(5) << "collision notify bounds " << bounds << "changed";
   csg::Cube3 current_chunks = csg::GetChunkIndex(bounds, NavGridTile::TILE_SIZE);
   csg::Cube3 previous_chunks = csg::GetChunkIndex(last_bounds, NavGridTile::TILE_SIZE);

   for (csg::Point3 const& cursor : previous_chunks) {
      if (!current_chunks.Contains(cursor)) {
         NG_LOG(5) << "removing tracker to grid tile at " << cursor;
         GridTile(cursor).RemoveCollisionTracker(type, tracker);
      }
   }

   for (csg::Point3 const& cursor : current_chunks) {
      if (!previous_chunks.Contains(cursor)) {
         NG_LOG(5) << "adding tracker to grid tile at " << cursor;
         GridTile(cursor).AddCollisionTracker(type, tracker);
      }
   }
}

NavGridTile& NavGrid::GridTile(csg::Point3 const& pt) const
{
   auto i = nav_grid_.find(pt);
   if (i != nav_grid_.end()) {
      return i->second;
   }
   NG_LOG(5) << "constructing new grid tile at " << pt;
   auto j = nav_grid_.insert(std::make_pair(pt, NavGridTile(const_cast<NavGrid&>(*this), pt))).first;
   return j->second;
}

void NavGrid::ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg)
{
   csg::Point3 index = csg::GetChunkIndex(pt, NavGridTile::TILE_SIZE);
   auto i = nav_grid_.find(index);
   if (i != nav_grid_.end()) {
      i->second.ShowDebugShapes(pt - i->first, msg);
   } else {
      NG_LOG(1) << "no navgrid tile at world coordinate " << pt << "!";
   }
}

