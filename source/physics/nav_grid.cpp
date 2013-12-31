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

NavGrid::NavGrid(int trace_category) :
   trace_category_(trace_category),
   bounds_(csg::Cube3::zero)
{
}

/*
 * -- NavGrid::TrackComponent
 *
 * Notifies the NavGrid that a new component has been added to some entity in the system.
 * If we care, create a CollisionTracker object to keep track of it.  See collision_tracker.h
 * for details.
 */
void NavGrid::TrackComponent(std::shared_ptr<dm::Object> component)
{
   dm::ObjectId id = component->GetObjectId();

   switch (component->GetObjectType()) {
   case om::TerrainObjectType: {
      auto terrain = std::static_pointer_cast<om::Terrain>(component);
      CollisionTrackerPtr tracker = std::make_shared<TerrainTracker>(*this, terrain->GetEntityPtr(), terrain);
      collision_trackers_[id] = tracker;
      tracker->Initialize();
      break;
   }
   case om::RegionCollisionShapeObjectType: {
      auto rcs = std::static_pointer_cast<om::RegionCollisionShape>(component);
      CollisionTrackerPtr tracker = std::make_shared<RegionCollisionShapeTracker>(*this, rcs->GetEntityPtr(), rcs);
      collision_trackers_[id] = tracker;
      tracker->Initialize();
      break;
   }
   case om::VerticalPathingRegionObjectType: {
      auto rcs = std::static_pointer_cast<om::VerticalPathingRegion>(component);
      CollisionTrackerPtr tracker = std::make_shared<VerticalPathingRegionTracker>(*this, rcs->GetEntityPtr(), rcs);
      collision_trackers_[id] = tracker;
      tracker->Initialize();
      break;
   }
   }
}

/*
 * -- NavGrid::AddTerrainTileTracker
 *
 * Helper function for the TerrainCollisionTracker.  This will be called once for every terrain
 * tile.
 */
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

/*
 * -- NavGrid::IsValidStandingRegion
 *
 * Return whether the collision shape specified is entirely supported by some other
 * collision shape  (i.e. whether it can be placed there and not fall over).  Is
 * useful (for example) for placing complicated objects like trees on terrain in
 * such a way that they're not partially hanging off the edge of a cliff and not
 * overlapping other trees.
 */
bool NavGrid::IsValidStandingRegion(csg::Region3 const& collision_shape) const
{
   for (csg::Cube3 const& cube : collision_shape) {
      if (!CanStandOn(cube)) {
         return false;
      }
   }
   return true;
}

/*
 * -- NavGrid::RemoveNonStandableRegion
 *
 * Given an entity and a region, remove the parts of the region that the
 * entity cannot stand on.  This is useful to the valid places an entity
 * can stand given a potentially valid set of spots.
 *
 * xxx: we need a "non-walkable" region to clip against.  until that exists,
 * do the slow thing...
 */
void NavGrid::RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& region) const
{
   csg::Region3 r2;
   for (csg::Cube3 const& cube : region) {
      for (csg::Point3 const& pt : cube) {
         if (CanStandOn(entity, pt)) {
            r2.AddUnique(pt);
         }
      }
   }
   region = r2;
}

/*
 * -- NavGrid::CanStandOn
 *
 * Returns the entity can possibly stand on the specified point.
 */
bool NavGrid::CanStandOn(om::EntityPtr entity, csg::Point3 const& pt) const
{
   csg::Point3 index, offset;
   csg::GetChunkIndex(pt, NavGridTile::TILE_SIZE, index, offset);
   
   if (!bounds_.Contains(pt)) {
      return false;
   }

   return GridTile(index).CanStandOn(offset);
}

/*
 * -- NavGrid::CanStandOn
 *
 * Returns the entity can possibly stand on the specified point.  This function
 * is private!  Never make it public please, as we want to be able to make
 * a decision based on the entity asking the question (e.g. critters will
 * be able to run through gated fences and titans will step over/through them,
 * but people won't.)
 *
 * xxx: this is horribly, horribly expensive!  we should do this
 * with region clipping or somethign...
 */
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

/*
 * -- NavGrid::CanStandOn
 *
 * Returns the trace category to use in TraceChanges, TraceDestroyed,
 * etc.
 */
int NavGrid::GetTraceCategory() const
{
   return trace_category_;
}

/*
 * -- NavGrid::AddCollisionTracker
 *
 * Helper function for CollisionTrackers used to notify the NavGrid that their collision
 * shape has changed.  This removes trackers from tiles which no longer need to know
 * about them and adds them to trackers which do.  This happens when regions are created,
 * moved around in the world, or when they grow or shrink.
 */
void NavGrid::AddCollisionTracker(NavGridTile::TrackerType type, csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker)
{
   NG_LOG(3) << "collision notify bounds " << bounds << "changed (last_bounds: " << last_bounds << ")";
   csg::Cube3 current_chunks = csg::GetChunkIndex(bounds, NavGridTile::TILE_SIZE);
   csg::Cube3 previous_chunks = csg::GetChunkIndex(last_bounds, NavGridTile::TILE_SIZE);

   bounds_.Grow(bounds.min - csg::Point3(-NavGridTile::TILE_SIZE, -NavGridTile::TILE_SIZE, -NavGridTile::TILE_SIZE));
   bounds_.Grow(bounds.max + csg::Point3(NavGridTile::TILE_SIZE, NavGridTile::TILE_SIZE, NavGridTile::TILE_SIZE));

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : previous_chunks) {
      if (!current_chunks.Contains(cursor)) {
         NG_LOG(5) << "removing tracker from grid tile at " << cursor;
         GridTile(cursor).RemoveCollisionTracker(type, tracker);
      }
   }

   // Add trackers to tiles which overlap the current bounds of the tracker.
   for (csg::Point3 const& cursor : current_chunks) {
      NG_LOG(5) << "adding tracker to grid tile at " << cursor;
      GridTile(cursor).AddCollisionTracker(type, tracker);
   }
}


/*
 * -- NavGrid::GridTile
 *
 * Returns the tile for the world-coordinate point pt, creating it if it does
 * not yet exist.  If you don't want to create the tile if the point is invalid,
 * use .find on tiles_ directly.
 */
NavGridTile& NavGrid::GridTile(csg::Point3 const& pt) const
{
   auto i = tiles_.find(pt);
   if (i != tiles_.end()) {
      return i->second;
   }
   NG_LOG(5) << "constructing new grid tile at " << pt;
   auto j = tiles_.emplace(pt, NavGridTile(const_cast<NavGrid&>(*this), pt)).first;
   return j->second;
}


/*
 * -- NavGrid::ShowDebugShapes
 *
 * Push some debugging information into the shaplist.
 */
void NavGrid::ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg)
{
   csg::Point3 index = csg::GetChunkIndex(pt, NavGridTile::TILE_SIZE);
   GridTile(index).ShowDebugShapes(msg);
}

