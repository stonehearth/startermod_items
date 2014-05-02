#include "radiant.h"
#include "radiant_stdutil.h"
#include "radiant_macros.h"
#include "nav_grid.h"
#include "nav_grid_tile.h"
#include "dm/store.h"
#include "dm/record_trace.h"
#include "csg/util.h"
#include "csg/color.h"
#include "core/config.h"
#include "om/components/component.h"
#include "om/components/mob.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "om/components/region_collision_shape.ridl.h"
#include "mob_tracker.h"
#include "terrain_tracker.h"
#include "terrain_tile_tracker.h"
#include "region_collision_shape_tracker.h"
#include "vertical_pathing_region_tracker.h"
#include "protocols/radiant.pb.h"

using namespace radiant;
using namespace radiant::phys;

static const int DEFAULT_WORKING_SET_SIZE = 128;

#define NG_LOG(level)              LOG(physics.navgrid, level)

NavGrid::NavGrid(int trace_category) :
   trace_category_(trace_category),
   bounds_(csg::Cube3::zero)
{
   last_evicted_ = 0;
   max_resident_ = core::Config::GetInstance().Get<int>("nav_grid.working_set_size", DEFAULT_WORKING_SET_SIZE);
   if (max_resident_ != DEFAULT_WORKING_SET_SIZE) {
      NG_LOG(1) << "working set size is " << max_resident_;
   }
   resident_tiles_.reserve(max_resident_);
   resident_tiles_.resize(0);
}

/*
 * -- NavGrid::TrackComponent
 *
 * Notifies the NavGrid that a new component has been added to some entity in the system.
 * If we care, create a CollisionTracker object to keep track of it.  See collision_tracker.h
 * for details.
 */
void NavGrid::TrackComponent(om::ComponentPtr component)
{
   dm::ObjectId id = component->GetObjectId();
   CollisionTrackerPtr tracker;
   switch (component->GetObjectType()) {
      case om::MobObjectType: {
         auto mob = std::static_pointer_cast<om::Mob>(component);
         if (mob->GetMobCollisionType() != om::Mob::NONE) {
            NG_LOG(7) << "creating MobTracker for " << *component->GetEntityPtr();
            tracker = std::make_shared<MobTracker>(*this, mob->GetEntityPtr(), mob);
         } else {
            NG_LOG(7) << "mob for " << *component->GetEntityPtr() << " has collision type NONE.  ignoring.";
         }
         break;
      }
      case om::TerrainObjectType: {
         NG_LOG(7) << "creating TerrainTracker for " << *component->GetEntityPtr();
         auto terrain = std::static_pointer_cast<om::Terrain>(component);
         tracker = std::make_shared<TerrainTracker>(*this, terrain->GetEntityPtr(), terrain);
         break;
      }
      case om::RegionCollisionShapeObjectType: {
         NG_LOG(7) << "creating RegionCollisionShapeTracker for " << *component->GetEntityPtr();
         auto rcs = std::static_pointer_cast<om::RegionCollisionShape>(component);
         tracker = std::make_shared<RegionCollisionShapeTracker>(*this, rcs->GetEntityPtr(), rcs);
         break;
      }
      case om::VerticalPathingRegionObjectType: {
         NG_LOG(7) << "creating VerticalPathingRegion for " << *component->GetEntityPtr();
         auto rcs = std::static_pointer_cast<om::VerticalPathingRegion>(component);
         tracker = std::make_shared<VerticalPathingRegionTracker>(*this, rcs->GetEntityPtr(), rcs);
         break;
      }
   }

   if (tracker) {
      collision_trackers_[id] = tracker;
      tracker->Initialize();
      collision_tracker_dtors_[id] = component->TraceChanges("nav grid destruction", GetTraceCategory())
                                          ->OnDestroyed([this, id]() {
                                             NG_LOG(3) << "tracker " << id << " destroyed.  updating grid.";
                                             collision_trackers_.erase(id);
                                             collision_tracker_dtors_.erase(id);
                                          });
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
bool NavGrid::IsValidStandingRegion(csg::Region3 const& collision_shape)
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
void NavGrid::RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& region)
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
bool NavGrid::CanStandOn(om::EntityPtr entity, csg::Point3 const& pt)
{
   csg::Point3 index, offset;
   csg::GetChunkIndex(pt, TILE_SIZE, index, offset);
   
   if (!bounds_.Contains(pt)) {
      return false;
   }

   NavGridTile& tile = GridTileResident(index);
   tile.FlushDirty(*this, index);

   ASSERT(tile.IsDataResident());
   return tile.CanStandOn(offset);
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
bool NavGrid::CanStandOn(csg::Cube3 const& cube)
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
int NavGrid::GetTraceCategory()
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
void NavGrid::AddCollisionTracker(csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker)
{
   NG_LOG(3) << "collision notify bounds " << bounds << "changed (last_bounds: " << last_bounds << ")";
   csg::Cube3 current_chunks = csg::GetChunkIndex(bounds, TILE_SIZE);
   csg::Cube3 previous_chunks = csg::GetChunkIndex(last_bounds, TILE_SIZE);

   bounds_.Grow(bounds.min - csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));
   bounds_.Grow(bounds.max + csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : previous_chunks) {
      if (!current_chunks.Contains(cursor)) {
         NG_LOG(5) << "removing tracker from grid tile at " << cursor;
         GridTileNonResident(cursor).RemoveCollisionTracker(tracker);
      }
   }

   // Add trackers to tiles which overlap the current bounds of the tracker.
   for (csg::Point3 const& cursor : current_chunks) {
      if (previous_chunks.Contains(cursor)) {
         NG_LOG(5) << "marking tracker to grid tile at " << cursor << " for " << *tracker->GetEntity() << " dirty";
         GridTileNonResident(cursor).OnTrackerChanged(tracker);
      } else {
         NG_LOG(5) << "adding tracker to grid tile at " << cursor << " for " << *tracker->GetEntity();
         GridTileNonResident(cursor).AddCollisionTracker(tracker);
      }
   }
}

/*
 * -- NavGrid::OnTrackerDestroyed
 *
 * Used to mark all the tiles overlapping bounds as dirty.  Useful for when a collision
 * tracker goes away.
 */
void NavGrid::OnTrackerDestroyed(csg::Cube3 const& bounds, dm::ObjectId entityId)
{
   NG_LOG(3) << "mark dirty " << bounds;
   csg::Cube3 chunks = csg::GetChunkIndex(bounds, TILE_SIZE);

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : chunks) {
      GridTileNonResident(cursor).OnTrackerRemoved(entityId);
   }
}

/*
 * -- NavGrid::GridTile
 *
 * Returns the tile for the world-coordinate point pt, creating it if it does
 * not yet exist, and make all the tile data resident (though not necessarily
 * up-to-date!).  (see NavGridTileData::CanStandOn)
 */
NavGridTile& NavGrid::GridTileResident(csg::Point3 const& pt)
{
   return GridTile(pt, true);
}

/*
 * -- NavGrid::GridTileNonResident
 *
 * Returns the tile for the world-coordinate point pt, creating it if it does
 * not yet exist.  This tile will not page in it's NavGridTileData, so it's
 * only safe to call functions on this tile which update the metadata (e.g.
 * AddCollisionTracker)
 */
NavGridTile& NavGrid::GridTileNonResident(csg::Point3 const& pt)
{
   return GridTile(pt, false);
}

/*
 * -- NavGrid::GridTile
 *
 * Returns the tile for the world-coordinate point pt, creating it if it does
 * not yet exist.  If you don't want to create the tile if the point is invalid,
 * use .find on tiles_ directly.
 */
NavGridTile& NavGrid::GridTile(csg::Point3 const& pt, bool make_resident)
{
   NavGridTileMap::iterator i = tiles_.find(pt);
   if (i == tiles_.end()) {
      NG_LOG(5) << "constructing new grid tile at " << pt;
      i = tiles_.insert(std::make_pair(pt, NavGridTile())).first;
   }
   NavGridTile& tile = i->second;

   if (make_resident) {
      if (tile.IsDataResident()) {
         for (auto &entry : resident_tiles_) {
            if (entry.first == pt) {
               entry.second = true;
               break;
            }
         }
      } else {
         NG_LOG(3) << "making nav grid tile " << pt << " resident";
         tile.SetDataResident(true);
         if (resident_tiles_.size() >= max_resident_) {
            EvictNextUnvisitedTile(pt);
         } else {
            resident_tiles_.push_back(std::make_pair(pt, true));
         }
      }
   }

   return tile;
}

/*
 * -- NavGrid::ShowDebugShapes
 *
 * Push some debugging information into the shaplist.
 */ 
void NavGrid::ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg)
{
   csg::Point3 index = csg::GetChunkIndex(pt, TILE_SIZE);
   NavGridTile& tile = GridTileResident(index);
   tile.FlushDirty(*this, index);
   tile.ShowDebugShapes(msg, index);

   csg::Color4 border_color(0, 0, 128, 64);
   csg::Color4 mob_color(128, 0, 128, 64);
   for (auto const& entry : resident_tiles_) {
      protocol::box* box = msg->add_box();
      csg::Point3f offset = csg::ToFloat(entry.first.Scaled(TILE_SIZE)) - csg::Point3f(0.5f, 0, 0.5f);
      offset.SaveValue(box->mutable_minimum());
      (offset + csg::Point3f(TILE_SIZE, TILE_SIZE, TILE_SIZE)).SaveValue(box->mutable_maximum());
      border_color.SaveValue(box->mutable_color());
   }
   auto i = collision_trackers_.begin(), end = collision_trackers_.end();
   for (; i != end; i++) {
      CollisionTrackerPtr tracker = i->second;
      if (tracker->GetType() == MOB) {
         MobTrackerPtr mob = std::static_pointer_cast<MobTracker>(tracker);
         csg::Point3f location = csg::ToFloat(mob->GetLastBounds().min) - csg::Point3f(0.5f, 0, 0.5f);
         protocol::box* box = msg->add_box();
         location.SaveValue(box->mutable_minimum());
         (location + csg::Point3f::one).SaveValue(box->mutable_maximum());
         mob_color.SaveValue(box->mutable_color());
      }
   };
}

/*
 * -- NavGrid::EvictNextUnvisitedTile
 *
 * Evict the NavGridTileData for the tile we're least likely to need in the near
 * future.  The last_evicted_ iterator points to the tile immediately after
 * the last tile that got evicted the last time.  We approximate the LRU tile by
 * sweeping last_evicted_ through the resident_tiles_ map, clearing visited bits
 * along the way, until we reach a tile that has not yet been visited.  This
 * is a pretty good approximation.
 *
 * (see http://en.wikipedia.org/wiki/Page_replacement_algorithm#Clock)
 */ 
void NavGrid::EvictNextUnvisitedTile(csg::Point3 const& pt)
{
   while (true) {
      if (last_evicted_ == resident_tiles_.size() - 1) {
         last_evicted_ = 0;
      }
      std::pair<csg::Point3, bool>& entry = resident_tiles_[last_evicted_];
      bool visited = entry.second;
      if (visited) {
         entry.second = false;
      } else {
         NG_LOG(3) << "evicted nav grid tile " << entry.first;
         GridTileNonResident(entry.first).SetDataResident(false);
         entry.first = pt;
         entry.second = true;
         last_evicted_++;
         return;
      }
      last_evicted_++;
   }
}

/*
 * -- NavGrid::ForEachEntityAtIndex
 *
 * Call the `cb` at least once (but perhaps more!) for all entities which overlap the tile
 * at the specified index.  See NavGridTile::ForEachTracker for a more complete explanation.
 */ 
void NavGrid::ForEachEntityAtIndex(csg::Point3 const& index, ForEachEntityCb cb)
{
   if (bounds_.Contains(index.Scaled(TILE_SIZE))) {
      GridTileNonResident(index).ForEachTracker([cb](CollisionTrackerPtr tracker) {
         cb(tracker->GetEntity());
      });
   }
}

/*
 * -- NavGrid::ForEachEntityInBounds
 *
 * Get entities with shapes that intersect the specified world space cube.  As with 
 * ForEachEntityAtIndex, entities may be returned more than once!
 */
void NavGrid::ForEachEntityInBounds(csg::Cube3 const& worldBounds, ForEachEntityCb cb)
{
   csg::Cube3 clippedWorldBounds = worldBounds.Intersection(bounds_);
   csg::Cube3 indexBounds = csg::GetChunkIndex(clippedWorldBounds, TILE_SIZE);

   for (csg::Point3 const& index : indexBounds) {
      ASSERT(bounds_.Contains(index.Scaled(TILE_SIZE)));

      GridTileNonResident(index).ForEachTracker([&worldBounds, cb](CollisionTrackerPtr tracker) {
         if (tracker->Intersects(worldBounds)) {
            cb(tracker->GetEntity());
         }
      });
   }
}
