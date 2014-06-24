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
#include "om/components/destination.ridl.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "om/components/region_collision_shape.ridl.h"
#include "mob_tracker.h"
#include "terrain_tracker.h"
#include "terrain_tile_tracker.h"
#include "derived_region_tracker.h"
#include "protocols/radiant.pb.h"
#include "physics_util.h"

using namespace radiant;
using namespace radiant::phys;

static const int DEFAULT_WORKING_SET_SIZE = 128;

#define NG_LOG(level)              LOG(physics.navgrid, level)

NavGrid::NavGrid(dm::TraceCategories trace_category) :
   trace_category_(trace_category),
   bounds_(csg::Cube3::zero),
   _dirtyTilesSlot("dirty tiles")
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
   dm::ObjectId componentId = component->GetObjectId();
   om::EntityPtr entity = component->GetEntityPtr();
   CollisionTrackerPtr tracker;

   switch (component->GetObjectType()) {
      case om::MobObjectType: {
         auto mob = std::static_pointer_cast<om::Mob>(component);
         if (mob->GetMobCollisionType() != om::Mob::NONE) {
            NG_LOG(7) << "creating MobTracker for " << *entity;
            tracker = std::make_shared<MobTracker>(*this, entity, mob);
         } else {
            NG_LOG(7) << "mob for " << *entity << " has collision type NONE.  ignoring.";
         }
         break;
      }
      case om::TerrainObjectType: {
         NG_LOG(7) << "creating TerrainTracker for " << *entity;
         auto terrain = std::static_pointer_cast<om::Terrain>(component);
         tracker = std::make_shared<TerrainTracker>(*this, entity, terrain);
         break;
      }
      case om::RegionCollisionShapeObjectType: {
         auto rcs = std::static_pointer_cast<om::RegionCollisionShape>(component);
         tracker = CreateRegionCollisonShapeTracker(rcs);
         CreateCollisionTypeTrace(rcs);
         break;
      }
      case om::DestinationObjectType: {
         NG_LOG(7) << "creating DestinationRegionTracker for " << *entity;
         auto dst = std::static_pointer_cast<om::Destination>(component);
         tracker = std::make_shared<DestinationRegionTracker>(*this, entity, dst);
         break;
      }
      case om::VerticalPathingRegionObjectType: {
         NG_LOG(7) << "creating VerticalPathingRegionTracker for " << *entity;
         auto vpr = std::static_pointer_cast<om::VerticalPathingRegion>(component);
         tracker = std::make_shared<VerticalPathingRegionTracker>(*this, entity, vpr);
         break;
      }
   }

   if (tracker) {
      AddComponentTracker(tracker, component);
   }
}

/*
 * -- NavGrid::AddComponentTracker
 *
 * Add a tracker for a region managed by a component.
 */
void NavGrid::AddComponentTracker(CollisionTrackerPtr tracker, om::ComponentPtr component)
{
   if (tracker) {
      dm::ObjectId componentId = component->GetObjectId();
      dm::ObjectType componentType = component->GetObjectType();
      om::EntityPtr entity = component->GetEntityPtr();
      dm::ObjectId entityId = entity->GetObjectId();

      collision_trackers_[entityId][componentId] = tracker;
      tracker->Initialize();
      collision_tracker_dtors_[componentId] = component->TraceChanges("nav grid destruction", GetTraceCategory())
                                                       ->OnDestroyed([this, entityId, componentId, componentType]() {
                                                          RemoveComponentTracker(entityId, componentId);
                                                       });
   }
}

/*
 * -- NavGrid::RemoveComponentTracker
 *
 * Remove a tracker for a region managed by a component. Note that the destructor for the
 * tracker should call NavGrid::OnTrackerDestroyed to set the dirty bits on the grid tiles.
 */
void NavGrid::RemoveComponentTracker(dm::ObjectId entityId, dm::ObjectId componentId)
{
   NG_LOG(3) << "tracker " << componentId << " destroyed.  updating grid.";
   CollisionTrackerPtr keepAlive = collision_trackers_[entityId][componentId];
   collision_trackers_[entityId].erase(componentId);
   collision_tracker_dtors_.erase(componentId);
}

/*
 * -- NavGrid::CreateRegionCollisonShapeTracker
 *
 * Create a tracker for the RegionCollisionShape based on the RegionCollisionType.
 */
CollisionTrackerPtr NavGrid::CreateRegionCollisonShapeTracker(std::shared_ptr<om::RegionCollisionShape> regionCollisionShapePtr)
{
   om::EntityPtr entity = regionCollisionShapePtr->GetEntityPtr();
   auto regionCollisionType = regionCollisionShapePtr->GetRegionCollisionType();

   switch (regionCollisionType) {
      case om::RegionCollisionShape::RegionCollisionTypes::SOLID:
         NG_LOG(7) << "creating RegionCollisionShapeTracker for " << *entity;
         return std::make_shared<RegionCollisionShapeTracker>(*this, entity, regionCollisionShapePtr);
         break;
      default:
         ASSERT(regionCollisionType == om::RegionCollisionShape::RegionCollisionTypes::NONE);
         NG_LOG(7) << "creating RegionNonCollisionShapeTracker for " << *entity;
         return std::make_shared<RegionNonCollisionShapeTracker>(*this, entity, regionCollisionShapePtr);
   }
}

/*
 * -- NavGrid::CreateCollisionTypeTrace
 *
 * Create and add the trace for the RegionCollisionType.
 */
void NavGrid::CreateCollisionTypeTrace(std::shared_ptr<om::RegionCollisionShape> regionCollisionShapePtr)
{
   dm::ObjectId componentId = regionCollisionShapePtr->GetObjectId();
   NG_LOG(7) << "creating trace for RegionCollisionType on component " << componentId;
   auto trace = regionCollisionShapePtr->TraceRegionCollisionType("nav grid", GetTraceCategory());
   std::weak_ptr<om::RegionCollisionShape> regionCollisionShapeRef = regionCollisionShapePtr;

   trace->OnModified([this, regionCollisionShapeRef]() {
      OnCollisionTypeChanged(regionCollisionShapeRef);
   });
   trace->OnDestroyed([this, componentId]() {
      NG_LOG(7) << "removing trace for RegionCollisionType on component " << componentId;
      collision_type_traces_.erase(componentId);
   });
   collision_type_traces_[componentId] = trace;
}

/*
 * -- NavGrid::OnCollisionTypeChanged
 *
 * Called when the RegionCollisionType changes.
 */
void NavGrid::OnCollisionTypeChanged(std::weak_ptr<om::RegionCollisionShape> regionCollisionShapeRef)
{
   std::shared_ptr<om::RegionCollisionShape> regionCollisionShapePtr = regionCollisionShapeRef.lock();

   if (regionCollisionShapePtr) {
      dm::ObjectId componentId = regionCollisionShapePtr->GetObjectId();
      om::EntityPtr entity = regionCollisionShapePtr->GetEntityPtr();
      dm::ObjectId entityId = entity->GetObjectId();

      NG_LOG(7) << "RegionCollisionType changed on " << *entity;
      RemoveComponentTracker(entityId, componentId);
      CollisionTrackerPtr tracker = CreateRegionCollisonShapeTracker(regionCollisionShapePtr);
      AddComponentTracker(tracker, regionCollisionShapePtr);
   }
}

/*
 * -- NavGrid::AddTerrainTileTracker
 *
 * Terrain tile trackers are added differently than the other trackers.
 * This will be called once for every terrain tile.
 */
void NavGrid::AddTerrainTileTracker(om::EntityRef e, csg::Point3 const& offset, om::Region3BoxedPtr tile)
{
   NG_LOG(3) << "tracking terrain tile at " << offset;
   om::EntityPtr entity = e.lock();
   if (entity) {
      CollisionTrackerPtr tracker = std::make_shared<TerrainTileTracker>(*this, entity, offset, tile);
      terrain_tile_collision_trackers_[offset] = tracker;
      tracker->Initialize();
   }
}

/*
 * -- NavGrid::RemoveNonStandableRegion
 *
 * Given an entity and a region, remove the parts of the region that the
 * entity cannot stand on.  This is useful to the valid places an entity
 * can stand given a potentially valid set of spots.
 *
 * `region` must be specified in world coordinates.  This is intentional:
 * if you're specifing in something in local coordinates, you probably
 * haven't considered all cases of when the region might be rotated!
 *
 */
void NavGrid::RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3& region)
{
   csg::Region3 nonStandable;

   for (csg::Cube3 const& cube : region) {
      for (csg::Point3 const& pt : cube) {
         if (!IsStandable(pt)) {
            nonStandable.AddUnique(pt);
         }
      }
   }

   region -= nonStandable;
}

/*
 * -- NavGrid::GetTraceCategory
 *
 * Returns the trace category to use in TraceChanges, TraceDestroyed,
 * etc.
 */
dm::TraceCategories NavGrid::GetTraceCategory()
{
   return trace_category_;
}

/*
 * -- NavGrid::OnTrackerBoundsChanged
 *
 * Helper function for CollisionTrackers used to notify the NavGrid that their collision
 * shape has changed.  This removes trackers from tiles which no longer need to know
 * about them and adds them to trackers which do.  This happens when regions are created,
 * moved around in the world, or when they grow or shrink.
 */
void NavGrid::OnTrackerBoundsChanged(csg::Cube3 const& last_bounds, csg::Cube3 const& bounds, CollisionTrackerPtr tracker)
{
   NG_LOG(3) << "collision tracker bounds " << bounds << " changed (last_bounds: " << last_bounds << ")";
   csg::Cube3 current_chunks = csg::GetChunkIndex(bounds, TILE_SIZE);
   csg::Cube3 previous_chunks = csg::GetChunkIndex(last_bounds, TILE_SIZE);

   csg::Point3 chunkBoundsMin = current_chunks.min.Scaled(TILE_SIZE);
   csg::Point3 chunkBoundsMax = current_chunks.max.Scaled(TILE_SIZE);

   bounds_.Grow(chunkBoundsMin);
   bounds_.Grow(chunkBoundsMax);

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
      NG_LOG(5) << "adding tracker for grid tile at " << cursor << " for " << *tracker->GetEntity();
      GridTileNonResident(cursor).AddCollisionTracker(tracker);
   }
}

/*
 * -- NavGrid::OnTrackerDestroyed
 *
 * Used to mark all the tiles overlapping bounds as dirty.  Useful for when a collision
 * tracker goes away.
 */
void NavGrid::OnTrackerDestroyed(csg::Cube3 const& bounds, dm::ObjectId entityId, TrackerType type)
{
   NG_LOG(3) << "mark dirty " << bounds;
   csg::Cube3 chunks = csg::GetChunkIndex(bounds, TILE_SIZE);

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : chunks) {
      GridTileNonResident(cursor).OnTrackerRemoved(entityId, type);
   }
}

/*
 * -- NavGrid::GridTile
 *
 * Returns the tile for the world-coordinate point pt, creating it if it does
 * not yet exist, and make all the tile data resident.
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
NavGridTile& NavGrid::GridTile(csg::Point3 const& index, bool make_resident)
{
   NavGridTileMap::iterator i = tiles_.find(index);
   if (i == tiles_.end()) {
      NG_LOG(5) << "constructing new grid tile at " << index;
      i = tiles_.emplace(std::make_pair(index, NavGridTile(*this, index))).first;
   }
   NavGridTile& tile = i->second;

   if (make_resident) {
      if (tile.IsDataResident()) {
         for (auto &entry : resident_tiles_) {
            if (entry.first == index) {
               entry.second = true;
               break;
            }
         }
      } else {
         NG_LOG(3) << "making nav grid tile " << index << " resident";
         tile.SetDataResident(true);
         if (resident_tiles_.size() >= max_resident_) {
            EvictNextUnvisitedTile(index);
         } else {
            resident_tiles_.push_back(std::make_pair(index, true));
         }
      }
      tile.FlushDirty(*this, index);
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

   // Render all the standable locations in the block pointed to by `pt`
   csg::Color4 standable_color(0, 0, 255, 64);
   for (csg::Point3 i : csg::Cube3::one.Scaled(TILE_SIZE).Translated(index * TILE_SIZE)) {
      if (IsStandable(i)) {
         protocol::coord* coord = msg->add_coords();
         i.SaveValue(coord);
         standable_color.SaveValue(coord->mutable_color());
      }
   }

   // Draw a box around all the resident tiles.
   csg::Color4 border_color(0, 0, 128, 64);
   csg::Color4 mob_color(128, 0, 128, 64);
   for (auto const& entry : resident_tiles_) {
      protocol::box* box = msg->add_box();
      csg::Point3f offset = csg::ToFloat(entry.first.Scaled(TILE_SIZE)) - csg::Point3f(0.5f, 0, 0.5f);
      offset.SaveValue(box->mutable_minimum());
      (offset + csg::Point3f(TILE_SIZE, TILE_SIZE, TILE_SIZE)).SaveValue(box->mutable_maximum());
      border_color.SaveValue(box->mutable_color());
   }

   // Draw a purple box around all the mobs
   auto i = collision_trackers_.begin(), end = collision_trackers_.end();
   for (; i != end; i++) {
      for (auto const& entry : i->second) {
         CollisionTrackerPtr tracker = entry.second;
         if (tracker->GetType() == MOB) {
            MobTrackerPtr mob = std::static_pointer_cast<MobTracker>(tracker);
            csg::Cube3f bounds = csg::ToFloat(mob->GetBounds()).Translated(-csg::Point3f(0.5f, 0, 0.5f));
            protocol::box* box = msg->add_box();
            bounds.min.SaveValue(box->mutable_minimum());
            bounds.max.SaveValue(box->mutable_maximum());
            mob_color.SaveValue(box->mutable_color());
         }
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
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */ 
bool NavGrid::ForEachEntityAtIndex(csg::Point3 const& index, ForEachEntityCb cb)
{
   bool stopped = false;
   if (bounds_.Contains(index.Scaled(TILE_SIZE))) {
      stopped = GridTileNonResident(index).ForEachTracker([cb](CollisionTrackerPtr tracker) {
         ASSERT(tracker);
         ASSERT(tracker->GetEntity());
         bool stop = cb(tracker->GetEntity());
         return stop;
      });
   }
   return stopped;
}

/*
 * -- NavGrid::ForEachEntityInBounds
 *
 * Get entities with shapes that intersect the specified world space cube.  As with 
 * ForEachEntityAtIndex, entities may be returned more than once!
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */
bool NavGrid::ForEachEntityInBounds(csg::Cube3 const& worldBounds, ForEachEntityCb cb)
{
   bool stopped;
   stopped = ForEachTileInBounds(worldBounds, [&worldBounds, cb](csg::Point3 const& index, NavGridTile &tile) {
      return tile.ForEachTracker([&worldBounds, cb](CollisionTrackerPtr tracker) {
         bool stop = false;
         if (tracker->Intersects(worldBounds)) {
            stop = cb(tracker->GetEntity());
         }
         return stop;
      });
   });
   return stopped;
}


/*
 * -- NavGrid::ForEachEntityInRegion
 *
 * Iterate over every Entity in the region.  Guarantees to visit an entity no more
 * than once.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */
bool NavGrid::ForEachEntityInRegion(csg::Region3 const& region, ForEachEntityCb cb)
{
   bool stopped;
   std::set<dm::ObjectId> visited;

   stopped = ForEachTrackerInRegion(region, [&visited, cb](CollisionTrackerPtr tracker) -> bool {
      bool stop = false;
      om::EntityPtr entity = tracker->GetEntity();
      if (entity) {
         if (visited.insert(entity->GetObjectId()).second) {
            stop = cb(entity);
         }
      }
      return stop;
   });
   return stopped;
}


/*
 * -- NavGrid::ForEachTileInBounds
 *
 * Iterate over every tile which intersects the specified `worldBounds`.  This tile is
 * not resident, so you may get an error if you try to check the nav grid tile data in
 * any way.  The index is passed along with the tile to the callback so you can call
 * ::GetTileResident(index) if you'd rather have a resident tile.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */
bool NavGrid::ForEachTileInBounds(csg::Cube3 const& worldBounds, ForEachTileCb cb)
{
   bool stopped = false;
   csg::Cube3 clippedWorldBounds = worldBounds.Intersection(bounds_);
   csg::Cube3 indexBounds = csg::GetChunkIndex(clippedWorldBounds, TILE_SIZE);

   auto i = indexBounds.begin(), end = indexBounds.end();
   while (!stopped && i != end) {
      csg::Point3 index = *i;
      stopped = cb(index, GridTileNonResident(index));
      ++i;
   }
   return stopped;
}


/*
 * -- NavGrid::ForEachTileInRegion
 *
 * Iterate over every tile which intersects the specified `region`, in world coordinates.
 * This tile is not resident, so you may get an error if you try to check the nav grid tile
 * data in any way.  The index is passed along with the tile to the callback so you can call
 * ::GetTileResident(index) if you'd rather have a resident tile.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */

bool NavGrid::ForEachTileInRegion(csg::Region3 const& region, ForEachTileCb cb)
{
   bool stopped = false;
   std::set<csg::Point3> visited;

   auto i = region.begin(), end = region.end();
   while (!stopped && i != end) {
      csg::Cube3 const& cube = *i++;
      stopped = ForEachTileInBounds(cube, [&region, &visited, cb](csg::Point3 const& index, NavGridTile &tile) {
         bool stop = false;
         if (visited.insert(index).second) {
            stop = cb(index, tile);
         }
         return stop;
      });
   }
   return stopped;
}


/*
 * -- NavGrid::ForEachTrackerInRegion
 *
 * Iterate over every tracker which intersects the specified `region`, in world coordinates.
 * Each tracker is guaranteed to be passed to `cb` only once, regardless of how many tiles
 * it's registered in.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */

bool NavGrid::ForEachTrackerInRegion(csg::Region3 const& region, ForEachTrackerCb cb)
{
   bool stopped;
   std::set<CollisionTracker*> visited;
   csg::Cube3 regionBounds = region.GetBounds();

   stopped = ForEachTileInRegion(region, [&region, &regionBounds, &visited, cb](csg::Point3 const& index, NavGridTile& tile) {
      return tile.ForEachTracker([&region, &regionBounds, &visited, index, cb](CollisionTrackerPtr tracker) {
         bool stop = false;
         if (visited.insert(tracker.get()).second) {
            if (tracker->Intersects(region, regionBounds)) {
               stop = cb(tracker);
            }
         }
         return stop;
      });
   });

   return stopped;
}


/*
 * -- NavGrid::ForEachTrackerForEntity
 *
 * Iterate over every tracker for an Entity with the specified `entityId`.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */

bool NavGrid::ForEachTrackerForEntity(dm::ObjectId entityId, ForEachTrackerCb cb)
{
   bool stopped = false;
   auto i = collision_trackers_.find(entityId), end = collision_trackers_.end();

   if (!stopped && i != end) {
      for (auto const& entry : i->second) {
         CollisionTrackerPtr tracker = entry.second;
         stopped = cb(tracker);
      }
   }
   return stopped;
}


/*
 * -- NavGrid::ForEachPointInEntityRegion
 *
 * Iterate over every point of an Entity, which is defined to be the sum of all regions
 * of all the trackers for an Entity.  Points *may* be passed multiple times if trackers
 * overlap!!
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */

bool NavGrid::ForEachPointInEntityRegion(dm::ObjectId entityId, csg::Point3 const& offset, ForEachPointCb cb)
{
   bool stopped;
   stopped = ForEachTrackerForEntity(entityId, [&offset, cb] (CollisionTrackerPtr tracker) mutable {
      for (csg::Cube3 const& cube : tracker->GetLocalRegion()) {
         for (csg::Point3 pt : cube.Translated(offset)) {
            bool stop = cb(pt);
            return stop;
         }
      }
      return false;     // keep iterating...
   });
   return stopped;      // if we had to stop iteration, we must be blocked!
}


/*
 * -- NavGrid::IsEntityInCube
 *
 * Returns whether or any tracker for `entity` overlaps `worldBounds`
 *
 */

bool NavGrid::IsEntityInCube(om::EntityPtr entity, csg::Cube3 const& worldBounds)
{
   bool stopped;
   stopped = ForEachTrackerForEntity(entity->GetObjectId(), [worldBounds](CollisionTrackerPtr tracker) {
      bool stop = tracker->Intersects(worldBounds);
      return stop;
   });
   return stopped;   // if we had to stop early, we must have found an overlap!
}


/*
 * -- NavGrid::IsBlocked
 *
 * Returns whether or not the space occupied by `entity` is blocked if placed at
 * the world coordinate specified by `location`.  This completely ignores the
 * current position of the entity so it can be used for speculative queries (e.g.
 * if i *were* to move the entity here, would it be blocked?)
 *
 */

bool NavGrid::IsBlocked(om::EntityPtr entity, csg::Point3 const& location)
{
   bool stopped;

   NG_LOG(7) << "checking if " << *entity << " is blocked at " << location;
   stopped = ForEachPointInEntityRegion(entity->GetObjectId(), location, [&location, this] (csg::Point3 const& pt) mutable {
      bool stop = IsBlocked(pt);
      return stop;
   });
   NG_LOG(7) << "finished checking if " << *entity << " is blocked at " << location << "(result: " << std::boolalpha << stopped << ")";

   return stopped;      // if we had to stop iteration, we must be blocked!
}


/*
 * -- NavGrid::IsBlocked
 *
 * Returns whether or not the coordinate at `worldPoint` is blocked.
 *
 */

bool NavGrid::IsBlocked(csg::Point3 const& worldPoint)
{
   // Choose to do this with the resident tile data with the expectation that
   // anyone using the point API is actually iterating (so this is cheaper and faster
   // than the collision tracker method)
   csg::Point3 index, offset;
   csg::GetChunkIndex(worldPoint, TILE_SIZE, index, offset);
   bool blocked = GridTileResident(index).IsBlocked(offset);

   NG_LOG(7) << "checking if " << worldPoint << " is blocked (result:" << std::boolalpha << blocked << ")";
   return blocked;
}


/*
 * -- NavGrid::IsBlocked
 *
 * Returns whether or not any point in the specified `region` is blocked.  `region`
 * should be in the world coodinate system.
 *
 */
bool NavGrid::IsBlocked(csg::Region3 const& region) {
   NG_LOG(7) << "::IsBlocked checking world region " << region.GetBounds();
   bool blocked = ForEachTrackerInRegion(region, [](CollisionTrackerPtr tracker) {
      return tracker->GetType() == TrackerType::COLLISION; // we found one that's blocked!  stop!!
   });
   NG_LOG(7) << "::IsBlocked checking world region " << region.GetBounds() << "(result:" << std::boolalpha << blocked << ")";
   return blocked;   // we're blocked if we had to stop the iteration
}


/*
 * -- NavGrid::IsSupport
 *
 * Returns whether or not the coordinate at `worldPoint` is support.  A support
 * point is one that can be stood on by something else (e.g. a piece of ground or
 * the rung of a ladder).
 *
 */
bool NavGrid::IsSupport(csg::Point3 const& worldPoint)
{
   // Choose to do this with the resident tile data with the expectation that
   // anyone using the point API is actually iterating (so this is cheaper and faster
   // than the collision tracker method)
   csg::Point3 index, offset;
   csg::GetChunkIndex(worldPoint, TILE_SIZE, index, offset);
   return GridTileResident(index).IsSupport(offset);
}


/*
 * -- NavGrid::IsSupport
 *
 * Returns whether or not any point in the specified `region` is support.  `region`
 * should be in the world coodinate system.  A support point is one that can be stood
 * on by something else (e.g. a piece of ground or the rung of a ladder).
 *
 */
bool NavGrid::IsSupport(csg::Region3 const& r)
{
   NG_LOG(7) << "checking ::IsSupport for world region " << r.GetBounds();

   bool stopped = ForEachTileInRegion(r, [&r, this](csg::Point3 const& index, NavGridTile& tile) {
      // Clip the region to the tile bounds
      csg::Cube3 bounds = csg::Cube3::one.Scaled(TILE_SIZE).Translated(index * TILE_SIZE);
      csg::Region3 clipped = r & bounds;
      clipped.Translate(index * -TILE_SIZE);
      bool stop = !GridTileResident(index).IsSupport(clipped);
      return stop;
   });

   NG_LOG(7) << "checking ::IsSupport for world region " << r.GetBounds() << " (result: " << std::boolalpha << !stopped << ")";

   return !stopped;
}

/*
 * -- NavGrid::IsStandable
 *
 * Returns whether or not the coordinate at `worldPoint` is standable.  A point is
 * standable if it is not blocked and the point immediately below it is a
 * support point (i.e. something can stand there!)
 *
 */
bool NavGrid::IsStandable(csg::Point3 const& worldPoint)
{
   return !IsBlocked(worldPoint) && IsSupport(worldPoint - csg::Point3::unitY);
}

/*
 * -- NavGrid::IsStandable
 *
 * Returns whether or not the region `r` is a valid place to stand.  `r` should be
 * in world coordinates.  A Region3 is standable if the entire space occupied by
 * the region is not blocked and every point under the bottom-most slice of the
 * region is standable.
 *
 */
bool NavGrid::IsStandable(csg::Region3 const& r) {

   NG_LOG(7) << "::IsStandable checking world region " << r.GetBounds();

   if (IsBlocked(r)) {
      NG_LOG(7) << "region is blocked.  Definitely not standable!";
      return false;
   }

   csg::Cube3 rb = r.GetBounds();
   csg::Region3 footprint;

   int bottom = rb.min.y;
   for (csg::Cube3 const& cube : r) {
      // Only consider cubes that are on the bottom row of the region.
      if (cube.min.y == bottom) {
         csg::Cube3 slice(csg::Point3(cube.min.x, bottom - 1, cube.min.z),
                          csg::Point3(cube.max.x, bottom    , cube.max.z));
         footprint.AddUnique(slice);
      }
   }
   return IsSupport(footprint);
}


/*
 * -- NavGrid::IsBlocked
 *
 * Returns whether or not the space occupied by `entity` would be standable if placed at
 * the world coordinate specified by `location`.  This completely ignores the
 * current position of the entity so it can be used for speculative queries (e.g.
 * can the entity stand here?)
 *
 */

bool NavGrid::IsStandable(om::EntityPtr entity, csg::Point3 const& location)
{
   bool emptyRegion = true;
   bool stopped;
   stopped = ForEachPointInEntityRegion(entity->GetObjectId(), location, [this, &emptyRegion] (csg::Point3 const& pt) mutable {
      bool stop = !IsStandable(pt);
      emptyRegion = false;
      return stop;
   });
   // if there were no points at all, at least make sure the *location* itself is standable.
   if (emptyRegion) {
      NG_LOG(3) << *entity << " has no points in region in IsStandable.  checking " << location << " directly.";
      return IsStandable(location);
   }
   return !stopped;      // if we had to stop iteration, we must not be standable!!
}


/*
 * -- NavGrid::GetStandablePoint
 *
 * Given an `entity` and a starting `pt` in the world coodrinate system, return
 * the closest point that `entity` can stand on with the same y-coordinate at
 * `pt`.  If the starting point is blocked, GetStandablePoint will search up
 * for the standable point, otherwise it will search down.  This has the effect
 * of making things "fall" to the ground when they're in mid-air and "float"
 * to the surface if accidently placed overlapping some collision shape (e.g.
 * 3 feet under ground!)
 *
 */

csg::Point3 NavGrid::GetStandablePoint(om::EntityPtr entity, csg::Point3 const& pt)
{
   csg::Region3 region = GetEntityWorldCollisionRegion(entity, pt);
   if (region.IsEmpty()) {
      region.AddUnique(pt);
   }

   NG_LOG(7) << "collision region for " << *entity << " at " << pt << " is " << region.GetBounds() << " in ::GetStandablePoint.";
   
   csg::Point3 location = pt;
   csg::Point3 direction(0, IsBlocked(location) ? 1 : -1, 0);

   NG_LOG(7) << "::GetStandablePoint search direction is " << direction;

   while (bounds_.Contains(location)) {
      NG_LOG(7) << "::GetStandablePoint checking location " << location;
      if (IsStandable(region)) {
         NG_LOG(7) << "Found it!  Breaking";
         break;
      }
      NG_LOG(7) << "Still not standable.  Searching...";
      region.Translate(direction);
      location += direction;
   }
   return location;
}

/*
 * -- NavGrid::NotifyTileDirty
 *
 * Request a notification on `cb` whenever a NavGridTile becomes dirty.  This
 * notification is synchronous.  It's EXTREMELY bad form to do anything interesting
 * in your callback, since Tiles dirty a lot in many situations.  Well behaving
 * clients will buffer dirty notifications and process them asynchronously, to
 * avoid both wasted computation when tiles become dirty repeatedly and to avoid
 * spiking the CPU.
 *
 */
core::Guard NavGrid::NotifyTileDirty(std::function<void(csg::Point3 const&)> cb)
{
   return _dirtyTilesSlot.Register(cb);
}

/*
 * -- NavGrid::SignalTileDirty
 *
 * Notification from a NavGridTile that it's collision region is dirty.  Simply
 * passes this information along to any clients of the  NavGrid who are interested.
 * via the `_dirtyTilesSlot` slot.
 *
 */
void NavGrid::SignalTileDirty(csg::Point3 const& index)
{
   _dirtyTilesSlot.Signal(index);
}


/*
 * -- NavGrid::GetEntityWorldCollisionRegion
 *
 * Get a region representing the collision shape of the `entity` at world location
 * `location`.  This shape is useful ONLY for doing queries about the world (e.g.
 * can an entity of this shape fit here?). 
 *
 */
csg::Region3 NavGrid::GetEntityWorldCollisionRegion(om::EntityPtr entity, csg::Point3 const& location)
{
   csg::Point3 pos(csg::Point3::zero);

   if (entity) {
      om::MobPtr mob = entity->AddComponent<om::Mob>();
      pos = csg::ToInt(mob->GetWorldTransform().position);
      if (mob) {
         switch (mob->GetMobCollisionType()) {
         case om::Mob::TINY:
            return csg::Region3(location);
            break;
         case om::Mob::HUMANOID:
            return csg::Region3(csg::Cube3(location, location + csg::Point3(1, 4, 1)));
            break;
         }
      }

      om::RegionCollisionShapePtr rcs = entity->GetComponent<om::RegionCollisionShape>();
      if (rcs) {
         om::Region3BoxedPtr shape = rcs->GetRegion();
         if (shape) {
            switch (rcs->GetRegionCollisionType()) {
            case om::RegionCollisionShape::SOLID:
               csg::Region3 region = shape->Get();
               region = LocalToWorld(region, entity);
               region.Translate(location - pos);
               return region;
            }
         }
      }
   }
   return csg::Region3();
}

