#include "radiant.h"
#include "radiant_stdutil.h"
#include "radiant_macros.h"
#include "nav_grid.h"
#include "nav_grid_tile.h"
#include "dm/store.h"
#include "dm/record_trace.h"
#include "csg/util.h"
#include "csg/color.h"
#include "csg/iterators.h"
#include "core/config.h"
#include "om/components/component.h"
#include "om/components/mob.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/components/vertical_pathing_region.ridl.h"
#include "om/components/region_collision_shape.ridl.h"
#include "om/components/movement_guard_shape.ridl.h"
#include "om/components/movement_modifier_shape.ridl.h"
#include "mob_tracker.h"
#include "terrain_tracker.h"
#include "terrain_tile_tracker.h"
#include "destination_tracker.h"
#include "vertical_pathing_region_tracker.h"
#include "region_collision_shape_tracker.h"
#include "movement_guard_shape_tracker.h"
#include "movement_modifier_shape_tracker.h"
#include "protocols/radiant.pb.h"
#include "physics_util.h"
#include <EASTL/fixed_set.h>

using namespace radiant;
using namespace radiant::phys;

static const int CALENDAR_TICKS_PER_SECOND = 9;
static const int CALENDAR_TICKS_PER_HOUR = CALENDAR_TICKS_PER_SECOND * 60 * 60;
static const float DEFAULT_TILE_EXPIRE_TIME = 24.0f;
static const int SMALL_ITEM_MAX_HEIGHT = 2;
static const int SMALL_ITEM_MAX_WIDTH = 4;

#define NG_LOG(level)              LOG(physics.navgrid, level)

static csg::CollisionShape GetEntityWorldCollisionShape(om::EntityPtr entity, csg::Point3 const& location);

static bool IsSmallObject(om::EntityPtr entity)
{
   if (entity) {
      if (entity->GetObjectId() != 1) {
         csg::CollisionShape region = GetEntityWorldCollisionShape(entity, csg::Point3::zero);
         csg::Point3f size = region.GetBounds().GetSize();
         return size.x <= SMALL_ITEM_MAX_WIDTH && size.y <= SMALL_ITEM_MAX_HEIGHT && size.z <= SMALL_ITEM_MAX_WIDTH;
      }
   }
   return false;
}

static om::Mob::MobCollisionTypes GetMobCollisionType(om::EntityPtr entity)
{
   if (entity) {
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         return mob->GetMobCollisionType();
      }
   }
   return om::Mob::NONE;
}

NavGrid::NavGrid(dm::TraceCategories trace_category) :
   trace_category_(trace_category),
   bounds_(csg::Cube3::zero),
   _dirtyTilesSlot("dirty tiles"),
   tileExpireTime_(0),
   cachedTile_(csg::Point3(INT_MIN, INT_MIN, INT_MAX), nullptr)
{
   float hours = core::Config::GetInstance().Get<float>("navgrid.tile_data_expire_time", DEFAULT_TILE_EXPIRE_TIME);
   if (hours != DEFAULT_TILE_EXPIRE_TIME) {
      NG_LOG(1) << "working set size is " << hours << " game hours.";
   }
   tileExpireTime_ = static_cast<int>(hours * CALENDAR_TICKS_PER_HOUR);
}


/*
 * -- NavGrid::~NavGrid
 *
 * Get rid of the trackers before the tracking data.  If we let the complier generated 
 * destructor try to do this, it might get the order wrong (e.g. destroy at tracker may signal
 * the dirty tile slot, which is a bad idea if the dirty tile slot is already GONE!!).
 */

NavGrid::~NavGrid()
{
   collision_trackers_.clear();
   collision_tracker_dtors_.clear();
   collision_type_traces_.clear();
   terrain_tile_collision_trackers_.clear();
   for (auto& entry : tiles_) {
      delete entry.second;
   }
   tiles_.clear();
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
         tracker = CreateRegionCollisionShapeTracker(rcs);
         CreateCollisionTypeTrace(rcs);
         break;
      }
      case om::MovementModifierShapeObjectType: {
         auto mms = std::static_pointer_cast<om::MovementModifierShape>(component);
         tracker = std::make_shared<MovementModifierShapeTracker>(*this, entity, mms);
         break;
      }
      case om::MovementGuardShapeObjectType: {
         auto mgs = std::static_pointer_cast<om::MovementGuardShape>(component);
         tracker = std::make_shared<MovementGuardShapeTracker>(*this, entity, mgs);
         break;
      }
      case om::DestinationObjectType: {
         NG_LOG(7) << "creating DestinationRegionTracker for " << *entity;
         auto dst = std::static_pointer_cast<om::Destination>(component);
         tracker = std::make_shared<DestinationTracker>(*this, entity, dst);
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
 * -- NavGrid::CreateRegionCollisionShapeTracker
 *
 * Create a tracker for the RegionCollisionShape based on the RegionCollisionType.
 */
CollisionTrackerPtr NavGrid::CreateRegionCollisionShapeTracker(om::RegionCollisionShapePtr regionCollisionShapePtr)
{
   om::EntityPtr entity = regionCollisionShapePtr->GetEntityPtr();
   auto regionCollisionType = regionCollisionShapePtr->GetRegionCollisionType();

   NG_LOG(7) << "creating RegionCollisionShapeTracker for " << *entity;
   switch (regionCollisionType) {
      case om::RegionCollisionShape::RegionCollisionTypes::SOLID:
         return std::make_shared<RegionCollisionShapeTracker>(*this, COLLISION, entity, regionCollisionShapePtr);
      case om::RegionCollisionShape::RegionCollisionTypes::NONE:
         return std::make_shared<RegionCollisionShapeTracker>(*this, NON_COLLISION, entity, regionCollisionShapePtr);
      case om::RegionCollisionShape::RegionCollisionTypes::PLATFORM:
         return std::make_shared<RegionCollisionShapeTracker>(*this, PLATFORM, entity, regionCollisionShapePtr);
   }
   return nullptr;
}

/*
 * -- NavGrid::CreateCollisionTypeTrace
 *
 * Create and add the trace for the RegionCollisionType.
 */
void NavGrid::CreateCollisionTypeTrace(om::RegionCollisionShapePtr regionCollisionShapePtr)
{
   dm::ObjectId componentId = regionCollisionShapePtr->GetObjectId();
   NG_LOG(7) << "creating trace for RegionCollisionType on component " << componentId;
   auto trace = regionCollisionShapePtr->TraceRegionCollisionType("nav grid", GetTraceCategory());
   om::RegionCollisionShapeRef regionCollisionShapeRef = regionCollisionShapePtr;

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
void NavGrid::OnCollisionTypeChanged(om::RegionCollisionShapeRef regionCollisionShapeRef)
{
   om::RegionCollisionShapePtr regionCollisionShapePtr = regionCollisionShapeRef.lock();

   if (regionCollisionShapePtr) {
      dm::ObjectId componentId = regionCollisionShapePtr->GetObjectId();
      om::EntityPtr entity = regionCollisionShapePtr->GetEntityPtr();
      dm::ObjectId entityId = entity->GetObjectId();

      NG_LOG(7) << "RegionCollisionType changed on " << *entity;
      RemoveComponentTracker(entityId, componentId);
      CollisionTrackerPtr tracker = CreateRegionCollisionShapeTracker(regionCollisionShapePtr);
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
      CollisionTrackerPtr tracker = std::make_shared<TerrainTileTracker>(*this, entity, tile);
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
void NavGrid::RemoveNonStandableRegion(om::EntityPtr entity, csg::Region3f& region)
{
   csg::Region3f nonStandable;

   for (csg::Point3 pt : csg::EachPoint(region)) {
      if (!IsStandable(csg::ToInt(pt))) {
         nonStandable.AddUnique(csg::ToFloat(pt));
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
void NavGrid::OnTrackerBoundsChanged(csg::CollisionBox const& last_bounds, csg::CollisionBox const& bounds, CollisionTrackerPtr tracker)
{
   NG_LOG(3) << "collision tracker bounds " << bounds << " changed (last_bounds: " << last_bounds << ")";

   csg::Cube3 current_chunks = csg::GetChunkIndex<TILE_SIZE>(csg::ToInt(bounds));
   csg::Cube3 previous_chunks = csg::GetChunkIndex<TILE_SIZE>(csg::ToInt(last_bounds));

   csg::Point3 chunkBoundsMin = current_chunks.min.Scaled(TILE_SIZE);
   csg::Point3 chunkBoundsMax = current_chunks.max.Scaled(TILE_SIZE);

   bounds_.Grow(chunkBoundsMin);
   bounds_.Grow(chunkBoundsMax);

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : csg::EachPoint(previous_chunks)) {
      if (!current_chunks.Contains(cursor)) {
         NG_LOG(5) << "removing tracker from grid tile at " << cursor;
         GridTile(cursor).RemoveCollisionTracker(tracker);
      }
   }

   // Add trackers to tiles which overlap the current bounds of the tracker.
   for (csg::Point3 const& cursor : csg::EachPoint(current_chunks)) {
      NG_LOG(5) << "adding tracker for grid tile at " << cursor << " for " << *tracker->GetEntity();
      GridTile(cursor).AddCollisionTracker(tracker);
   }
}

/*
 * -- NavGrid::OnTrackerDestroyed
 *
 * Used to mark all the tiles overlapping bounds as dirty.  Useful for when a collision
 * tracker goes away.
 */
void NavGrid::OnTrackerDestroyed(csg::CollisionBox const& bounds, dm::ObjectId entityId, TrackerType type)
{
   NG_LOG(3) << "tracker for entity " << entityId << " covering " << bounds << " destroyed";

   csg::Cube3 chunks = csg::GetChunkIndex<TILE_SIZE>(csg::ToInt(bounds));

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : csg::EachPoint(chunks)) {
      GridTile(cursor).OnTrackerRemoved(entityId, type);
   }

   // Signal the top tile, too.
   SignalTileDirty(csg::GetChunkIndex<TILE_SIZE>(csg::ToInt(bounds.max) + csg::Point3::unitY));
}

/*
 * -- NavGrid::GridTile
 *
 * Returns the tile for the world-coordinate point pt, creating it if it does
 * not yet exist.  If you don't want to create the tile if the point is invalid,
 * use .find on tiles_ directly.
 */

NavGridTile& NavGrid::GridTile(csg::Point3 const& index)
{
   NavGridTile* tile;
   if (cachedTile_.first == index) {
      tile = cachedTile_.second;
   } else {
      NavGridTileMap::iterator i = tiles_.find(index);
      if (i == tiles_.end()) {
         NG_LOG(5) << "constructing new grid tile at " << index;
         NavGridTile* tile = new NavGridTile(*this, index);
         i = tiles_.emplace(std::make_pair(index, tile)).first;
         nextTileToEvict_ = tiles_.end(); // inserting invalidates the iterator.
      }
      tile = i->second;
      cachedTile_ = std::make_pair(index, tile);
   }
   return *tile;
}

NavGridTile const& NavGrid::GridTile(csg::Point3 const& pt) const
{
   return const_cast<NavGrid*>(this)->GridTile(pt);
}

/*
 * -- NavGrid::NotifyTileDirty
 *
 * Notification from a NavGridTile that it has become resident.  Returns the time
 * at which the data will be evicted. 
 */
 int NavGrid::NotifyTileResident(NavGridTile* tile)
{
   return now_ + tileExpireTime_;
}

/*
 * -- NavGrid::ShowDebugShapes
 *
 * Push some debugging information into the shaplist.
 */ 
void NavGrid::ShowDebugShapes(csg::Point3 const& pt, om::EntityRef pawn, protocol::shapelist* msg)
{
   csg::Point3 index = csg::GetChunkIndex<TILE_SIZE>(pt);
   om::EntityPtr p = pawn.lock();

   // Render all the standable locations in the block pointed to by `pt`
   csg::Color4 standable_color;
   switch (GetMobCollisionType(p)) {
      case om::Mob::TITAN:
         standable_color = csg::Color4(128, 0, 0, 64);
         break;
      case om::Mob::HUMANOID:
         standable_color = csg::Color4(0, 0, 128, 64);
         break;
      default:
         standable_color = csg::Color4(0, 0, 255, 64);
         break;
   }
   csg::Cube3 tile = csg::Cube3::one.Scaled(TILE_SIZE).Translated(index * TILE_SIZE);

   Query q;
   if (p) {
      q = Query(this, p);
   }
   for (csg::Point3 i : csg::EachPoint(tile)) {
      bool standable = p != nullptr ? q.IsStandable(i) : IsStandable(i);
      if (standable) {
         protocol::coord* coord = msg->add_coords();
         i.SaveValue(coord);
         standable_color.SaveValue(coord->mutable_color());
      }
   }

   // Draw a box around all the resident tiles.
   csg::Color4 border_color(0, 0, 128, 64);
   csg::Color4 mob_color(128, 0, 128, 64);
   for (auto const& entry : tiles_) {
      NavGridTile* tile = entry.second;
      if (tile->IsDataResident()) {
         protocol::box* box = msg->add_box();
         csg::Point3f offset = csg::ToFloat(tile->GetIndex().Scaled(TILE_SIZE)) - csg::Point3f(0.5f, 0, 0.5f);
         offset.SaveValue(box->mutable_minimum());
         (offset + csg::Point3f(TILE_SIZE, TILE_SIZE, TILE_SIZE)).SaveValue(box->mutable_maximum());
         border_color.SaveValue(box->mutable_color());
      }
   }

   // Draw a purple box around all the mobs
   auto i = collision_trackers_.begin(), end = collision_trackers_.end();
   for (; i != end; ++i) {
      for (auto const& entry : i->second) {
         CollisionTrackerPtr tracker = entry.second;
         if (tracker->GetType() == MOB) {
            MobTrackerPtr mob = std::static_pointer_cast<MobTracker>(tracker);
            csg::Cube3f bounds = mob->GetWorldShape().GetBounds().Translated(-csg::Point3f(0.5f, 0, 0.5f));
            protocol::box* box = msg->add_box();
            bounds.min.SaveValue(box->mutable_minimum());
            bounds.max.SaveValue(box->mutable_maximum());
            mob_color.SaveValue(box->mutable_color());
         }
      }
   };
}

/*
 * -- NavGrid::UpdateGameTime
 *
 */ 
void NavGrid::UpdateGameTime(int now, int freq)
{
   now_ = now;

   // We want to make sure we get through the entire list in about 15 realtime seconds
   uint size = (uint)tiles_.size();
   if (size > 0) {
      uint toProcess = std::max((uint)1, (size / (1000 / freq) / 15));
      for (uint i = 0; i < toProcess; i++) {
         if (nextTileToEvict_ == tiles_.end()) {
            nextTileToEvict_ = tiles_.begin();
         }
         NavGridTile* tile = nextTileToEvict_->second;
         if (tile->GetDataExpireTime() < now_) {
            NG_LOG(5) << "evicting tile " << tile->GetIndex() << ".";
            tile->ClearTileData();
         }
         nextTileToEvict_++;
      }
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
bool NavGrid::ForEachEntityAtIndex(csg::Point3 const& index, ForEachEntityCb const& cb)
{
   bool stopped = false;
   if (bounds_.Contains(index.Scaled(TILE_SIZE))) {
      stopped = GridTile(index).ForEachTracker([&cb](CollisionTrackerPtr tracker) {
         ASSERT(tracker);
         auto e = tracker->GetEntity();
         ASSERT(e);
         bool stop = cb(e);
         return stop;
      });
   }
   return stopped;
}

/*
 * -- NavGrid::ForEachEntityInBox
 *
 * Get entities with shapes that intersect the specified world space cube.  As with 
 * ForEachEntityAtIndex, entities may be returned more than once!
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */
bool NavGrid::ForEachEntityInBox(csg::CollisionBox const& worldBox, ForEachEntityCb const &cb)
{
   bool stopped;
   stopped = ForEachTileInBox(worldBox, [&worldBox, cb](csg::Point3 const& index, NavGridTile &tile) {
      return tile.ForEachTracker([&worldBox, cb](CollisionTrackerPtr tracker) {
         bool stop = false;
         if (tracker->Intersects(worldBox)) {
            stop = cb(tracker->GetEntity());
         }
         return stop;
      });
   });
   return stopped;
}


/*
 * -- NavGrid::ForEachEntityInShape
 *
 * Iterate over every Entity in the region.  Guarantees to visit an entity no more
 * than once.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */
bool NavGrid::ForEachEntityInShape(csg::CollisionShape const& worldShape, ForEachEntityCb const &cb)
{
   bool stopped;
   eastl::fixed_set<dm::ObjectId, 64> visited;

   stopped = ForEachTrackerInShape(worldShape, [&visited, cb](CollisionTrackerPtr tracker) -> bool {
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
 * -- NavGrid::ForEachTileInBox
 *
 * Iterate over every tile which intersects the specified `worldBox`.  This tile is
 * not resident, so you may get an error if you try to check the nav grid tile data in
 * any way.  The index is passed along with the tile to the callback so you can call
 * ::GetTileResident(index) if you'd rather have a resident tile.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */
bool NavGrid::ForEachTileInBox(csg::CollisionBox const& worldBox, ForEachTileCb const& cb)
{
   bool stopped = false;
   csg::Cube3 clippedWorldBounds = csg::ToInt(worldBox).Intersected(bounds_);
   csg::Cube3 indexBounds = csg::GetChunkIndex<TILE_SIZE>(clippedWorldBounds);

   csg::Cube3PointIterator i(indexBounds), end;
   while (!stopped && i != end) {
      csg::Point3 index = *i;
      stopped = cb(index, GridTile(index));
      ++i;
   }
   return stopped;
}


/*
 * -- NavGrid::ForEachTileInShape
 *
 * Iterate over every tile which intersects the specified `worldShape`, in world coordinates.
 * This tile is not resident, so you may get an error if you try to check the nav grid tile
 * data in any way.  The index is passed along with the tile to the callback so you can call
 * ::GetTileResident(index) if you'd rather have a resident tile.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */

bool NavGrid::ForEachTileInShape(csg::CollisionShape const& worldShape, ForEachTileCb const &cb)
{
   bool stopped = false;
   eastl::fixed_set<csg::Point3, 8> visited;

   auto i = worldShape.GetContents().begin(), end = worldShape.GetContents().end();
   while (!stopped && i != end) {
      csg::CollisionBox worldBox = *i++;
      stopped = ForEachTileInBox(worldBox, [&visited, cb](csg::Point3 const& index, NavGridTile &tile) {
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
 * -- NavGrid::ForEachTrackerInShape
 *
 * Iterate over every tracker which intersects the specified `region`, in world coordinates.
 * Each tracker is guaranteed to be passed to `cb` only once, regardless of how many tiles
 * it's registered in.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */

bool NavGrid::ForEachTrackerInShape(csg::CollisionShape const& worldShape, ForEachTrackerCb const &cb)
{
   bool stopped;
   eastl::fixed_set<CollisionTracker*, 16> visited;
   csg::CollisionBox worldBox = worldShape.GetBounds();

   stopped = ForEachTileInShape(worldShape, [&worldShape, &worldBox, &visited, cb](csg::Point3 const& index, NavGridTile& tile) {
      return tile.ForEachTracker([&worldShape, &worldBox, &visited, index, cb](CollisionTrackerPtr tracker) {
         bool stop = false;
         if (visited.insert(tracker.get()).second) {
            if (tracker->Intersects(worldShape, worldBox)) {
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

bool NavGrid::ForEachTrackerForEntity(dm::ObjectId entityId, ForEachTrackerCb const& cb)
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
 * -- NavGrid::IsEntityInCube
 *
 * Returns whether or any tracker for `entity` overlaps `worldBounds`
 *
 */

bool NavGrid::IsEntityInCube(om::EntityPtr entity, csg::Cube3 const& worldBounds)
{
   bool stopped;
   csg::CollisionBox worldBox = csg::ToFloat(worldBounds);
   stopped = ForEachTrackerForEntity(entity->GetObjectId(), [&worldBox](CollisionTrackerPtr tracker) {
      bool stop = tracker->Intersects(worldBox);
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
   csg::CollisionShape region = GetEntityWorldCollisionShape(entity, location);
   if (!UseFastCollisionDetection(entity)) {
      return IsBlocked(entity, region);
   }
   return IsBlocked(csg::ToInt(region));
}


/*
 * -- NavGrid::IsBlocked
 *
 * Returns whether or not the space occupied by `entity` is blocked if placed at
 * the world coordinate specified by `location`.  `region` must have been derived
 * from a call to GetEntityWorldCollisionReginon, though it may be moved around
 * (eg. translated up and down to find the right spot to unstick an entity)
 *
 */

bool NavGrid::IsBlocked(om::EntityPtr entity, csg::CollisionShape const& region)
{
   bool ignoreSmallObjects = GetMobCollisionType(entity) == om::Mob::TITAN;

   NG_LOG(7) << "Entering ::IsBlocked() for " << entity;
   bool stopped = ForEachTrackerInShape(region, [this, ignoreSmallObjects, &entity](CollisionTrackerPtr tracker) -> bool {
      bool stop = false;
      
      // Ignore the trackers for the entity we're asking about.
      if (entity != tracker->GetEntity()) {
         switch (tracker->GetType()) {
         case TrackerType::COLLISION:
            NG_LOG(7) << entity <<  " intersected collision box of " << tracker->GetEntity() << ".  ::IsBlocked() returning true.";
            stop = !ignoreSmallObjects || !IsSmallObject(tracker->GetEntity());
            break;
         case TrackerType::TERRAIN:
            NG_LOG(7) << entity <<  " intersected terrain.  ::IsBlocked() returning true.";
            stop = true;
            break;
         }
      }
      return stop;
   });
   NG_LOG(7) << "Exiting ::IsBlocked() for " << entity << "(blocked? " << std::boolalpha << stopped << ")";

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
   csg::GetChunkIndex<TILE_SIZE>(worldPoint, index, offset);
   bool blocked = GridTile(index).IsBlocked(offset);

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
bool NavGrid::IsBlocked(csg::Region3 const& region)
{
   NG_LOG(7) << "::IsBlocked checking world region " << region.GetBounds();
   for (csg::Cube3 const& cube : csg::EachCube(region)) {
      if (IsBlocked(cube)) {
         return true;
      }
   }
   return false;
}


/*
 * -- NavGrid::IsBlocked
 *
 * Returns whether or not any point in the specified `cube` is blocked.  `cube`
 * should be in the world coodinate system.
 *
 */
bool NavGrid::IsBlocked(csg::Cube3 const& cube)
{
   csg::Cube3 stencil = csg::Cube3::one.Scaled(TILE_SIZE);
   csg::Cube3 chunks = csg::GetChunkIndex<TILE_SIZE>(cube);

   bool stopped = csg::PartitionCubeIntoChunks<TILE_SIZE>(cube, [this](csg::Point3 const& index, csg::Cube3 const& c) {
      bool stop = GridTile(index).IsBlocked(c);
      return stop;
   });
   return stopped;      // if we stopped, we're blocked!
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
   if (!bounds_.Contains(worldPoint)) {
      return false;
   }

   // Choose to do this with the resident tile data with the expectation that
   // anyone using the point API is actually iterating (so this is cheaper and faster
   // than the collision tracker method)
   csg::Point3 index, offset;
   csg::GetChunkIndex<TILE_SIZE>(worldPoint, index, offset);
   bool isSupport = GridTile(index).IsSupport(offset);
   return isSupport;
}

/*
 * -- NavGrid::IsSupported
 *
 * Returns whether or not the coordinate at `worldPoint` is supported.  A supported
 * point has a support point one y unit below it.
 *
 */
bool NavGrid::IsSupported(csg::Point3 const& worldPoint)
{
   return IsSupport(worldPoint - csg::Point3::unitY);
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
   if (!bounds_.Contains(worldPoint)) {
      return false;
   }
   return !IsBlocked(worldPoint) && IsSupported(worldPoint);
}


/*
 * -- NavGrid::IsOccupied
 *
 * Returns whether or not the coordinate at `worldPoint` is occupied.  Returns
 * true if any entity in the world (of any kind) overlaps the specified point.
 *
 */
bool NavGrid::IsOccupied(csg::Point3 const& worldPoint)
{
   bool stopped = false;

   if (bounds_.Contains(worldPoint)) {
      csg::Point3 index = csg::GetChunkIndex<TILE_SIZE>(worldPoint);
      stopped = GridTile(index).ForEachTracker([worldPoint](CollisionTrackerPtr tracker) {
         ASSERT(tracker);
         ASSERT(tracker->GetEntity());
         bool stop = tracker->Intersects(worldPoint);
         return stop;
      });
   }
   return stopped;
}

/*
 * -- NavGrid::IsOccupied
 *
 * Returns whether or not the space occupied by `entity` would have any other entities
 * if the entity were placed at worldPoint.  This completely ignores the
 * current position of the entity so it can be used for speculative queries.
 *
 */

bool NavGrid::IsOccupied(om::EntityPtr entity, csg::Point3 const& worldPoint)
{
   csg::CollisionShape shape = GetEntityWorldCollisionShape(entity, worldPoint);

   bool stopped = ForEachTrackerInShape(shape, [&entity](CollisionTrackerPtr tracker) -> bool {
      // Ignore the trackers for the entity we're asking about.
      bool stop = entity != tracker->GetEntity();
      return stop;
   });

   return stopped;
}

/*
 * -- NavGrid::IsStandable
 *
 * Returns whether or not the region `r` is a valid place to stand.  `r` should be
 * in world coordinates.  
 *
 */

bool NavGrid::IsStandable(csg::Region3 const& r) {

   NG_LOG(7) << "::IsStandable checking world region " << r.GetBounds();

   if (IsBlocked(r)) {
      NG_LOG(7) << "region is blocked.  Definitely not standable!";
      return false;
   }
   return RegionIsSupported(r);
}

/*
 * -- NavGrid::RegionIsSupported
 *
 * Returns whether or not the region `r` in world space is supported.
 *
 */

bool NavGrid::RegionIsSupported(om::EntityPtr entity, csg::Point3 const& location, csg::Region3 const& r)
{
   if (GetMobCollisionType(entity) == om::Mob::TITAN) {
      return RegionIsSupportedForTitan(r);
   }
   return RegionIsSupported(r);
}

/*
 * -- NavGrid::RegionIsSupportedForTitan
 *
 * Returns whether or not the region `r` in world space is supported.
 * Ideall, a Region3 is standable if the entire space occupied by
 * the region is not blocked and every point under the bottom-most slice of the
 * region is standable.  Unfortuantely, that runs into weird collision problems
 * and odd-nesss in non-ideal situations.
 *
 * So instead we say a region is supported if ANY of the points under the bottom
 * stripe of the region are supported.  sigh.
 *
 */

bool NavGrid::RegionIsSupportedForTitan(csg::Region3 const& r)
{
   for (csg::Cube3 const& cube : csg::EachCube(r)) {
      csg::Cube3 slice(csg::Point3(cube.min.x, cube.min.y - 1, cube.min.z),
                       csg::Point3(cube.max.x, cube.min.y    , cube.max.z));
      for (csg::Point3 const& pt : csg::EachPoint(slice)) {
         if (IsTerrain(pt)) {
            NG_LOG(7) << pt << "is support!  returnin true. (titan)";
            return true;
         }
      }
   }
   NG_LOG(7) << "is region supported for " << r.GetBounds() << " returning false. (titan)";
   return false;
}


/*
 * -- NavGrid::RegionIsSupported
 *
 * Returns whether or not the region `r` in world space is supported.
 * Ideall, a Region3 is standable if the entire space occupied by
 * the region is not blocked and every point under the bottom-most slice of the
 * region is standable.  Unfortuantely, that runs into weird collision problems
 * and odd-nesss in non-ideal situations.
 *
 * So instead we say a region is supported if ANY of the points under the bottom
 * stripe of the region are supported.  sigh.
 *
 */

bool NavGrid::RegionIsSupported(csg::Region3 const& r)
{
   csg::Cube3 rb = r.GetBounds();

   NG_LOG(7) << "checking is region supported for " << r.GetBounds();

   int bottom = rb.min.y;
   for (csg::Cube3 const& cube : csg::EachCube(r)) {
      // Only consider cubes that are on the bottom row of the region.
      if (cube.min.y == bottom) {
         csg::Cube3 slice(csg::Point3(cube.min.x, bottom - 1, cube.min.z),
                          csg::Point3(cube.max.x, bottom    , cube.max.z));
         for (csg::Point3 const& pt : csg::EachPoint(slice)) {
            if (IsSupport(pt)) {
               NG_LOG(7) << pt << "is support!  returnin true";
               return true;
            }
         }
      }
   }
   NG_LOG(7) << "is region supported for " << r.GetBounds() << " returning false";
   return false;
}

/*
 * -- NavGrid::IsStandable
 *
 * Returns whether or not the space occupied by `entity` would be standable if placed at
 * the world coordinate specified by `location`.  This completely ignores the
 * current position of the entity so it can be used for speculative queries (e.g.
 * can the entity stand here?)
 *
 */

bool NavGrid::IsStandable(om::EntityPtr entity, csg::Point3 const& location)
{
   return Query(this, entity).IsStandable(location);
}

NavGrid::Query::Query() :
   _ng(nullptr),
   _method(INVALID_QUERY)
{
}

NavGrid::Query::Query(NavGrid *ng, om::EntityPtr const& entity) :
   _ng(ng),
   _entity(entity),
   _method(INVALID_QUERY)
{
   if (entity) {
      _entity = entity;
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         if (mob->GetMobCollisionType() == om::Mob::HUMANOID) {
            _method = Query::HUMANOID;
            return;
         }
         if (mob->GetMobCollisionType() == om::Mob::TINY) {
            _method = Query::TINY;
            return;
         }
      }
      
      _lastQueryPoint = csg::Point3::zero;
      _worldCollisionShape = GetEntityWorldCollisionShape(entity, csg::Point3::zero);
      if (_worldCollisionShape.IsEmpty()) {
         _worldCollisionShape.AddUnique(csg::Point3f::zero);
      }

      if (!_ng->UseFastCollisionDetection(entity)) {
         _method = Query::INTERSECT_TRACKERS;
         return;
      }

      if (!_worldCollisionShape.IsEmpty()) {
         _method = Query::INTERSECT_NAVGRID;
         return;
      }

      _method = Query::POINT;
   }
}


void NavGrid::Query::MoveWorldCollisionShape(csg::Point3 const& location) const
{
   // _worldCollisionShape contains the shape of the entity position at
   // _lastQueryPoint.  In order to do this query, we need to move it
   // over by the difference between that and the location.
   csg::Point3 delta = location - _lastQueryPoint;
   if (delta != csg::Point3::zero) {
      _lastQueryPoint = location;
      _worldCollisionShape.Translate(csg::ToFloat(delta));
   }
}

bool NavGrid::Query::IsStandable(csg::Point3 const& location) const
{
   switch (_method) {
   case Query::INVALID_QUERY:
      return false;
   
   case Query::POINT:
      return _ng->IsStandable(location);
   
   case Query::TINY:
      return _ng->IsStandable(location) &&
             _ng->CanPassThrough(_entity, location);

   case Query::HUMANOID:
      return _ng->IsStandable(location) &&
             _ng->CanPassThrough(_entity, location) &&
             !_ng->IsBlocked(location + csg::Point3::unitY) &&
             !_ng->IsBlocked(location + csg::Point3::unitY + csg::Point3::unitY);
   
   case Query::INTERSECT_NAVGRID:
      MoveWorldCollisionShape(location);
      return _ng->IsStandable(csg::ToInt(_worldCollisionShape));
   
   case Query::INTERSECT_TRACKERS:
      MoveWorldCollisionShape(location);
      return _ng->IsStandable(_entity, location, _worldCollisionShape);
   
   default:
      ASSERT(false);
   }
   return false;
}

bool NavGrid::Query::IsBlocked(csg::Point3 const& location) const
{
   switch (_method) {
   case Query::INVALID_QUERY:
      return false;
   
   case Query::POINT:
      return _ng->IsBlocked(location);
   
   case Query::TINY:
      return _ng->IsBlocked(location);

   case Query::HUMANOID:
      return _ng->IsBlocked(location) ||
             _ng->IsBlocked(location + csg::Point3::unitY) ||
             _ng->IsBlocked(location + csg::Point3::unitY + csg::Point3::unitY);
   
   case Query::INTERSECT_NAVGRID:
      MoveWorldCollisionShape(location);
      return _ng->IsBlocked(csg::ToInt(_worldCollisionShape));
   
   case Query::INTERSECT_TRACKERS:
      MoveWorldCollisionShape(location);
      return _ng->IsBlocked(_entity, _worldCollisionShape);
   
   default:
      ASSERT(false);
   }
   return false;
}


/*
 * -- NavGrid::IsStandable
 *
 * Returns whether or not the space occupied by `entity` would be standable if placed at
 * the world coordinate specified by `region`.  `region` must have been derived
 * from a call to GetEntityWorldCollisionReginon, though it may be moved around
 * (eg. translated up and down to find the right spot to unstick an entity)
 *
 */

bool NavGrid::IsStandable(om::EntityPtr entity, csg::Point3 const& location, csg::CollisionShape const& worldShape)
{
   if (IsBlocked(entity, worldShape)) {
      NG_LOG(7) << "::IsStandable for " << entity << " returning false.  Shape is blocked!";
      return false;
   }
   return RegionIsSupported(entity, location, csg::ToInt(worldShape));
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
   csg::Point3 location = bounds_.GetClosestPoint(csg::ToClosestInt(pt));
   Query q = Query(this, entity);

   csg::Point3 direction(0, q.IsBlocked(pt) ? 1 : -1, 0);
   
   NG_LOG(7) << "collision region for " << *entity << " at " << location << " in ::GetStandablePoint.";  
   NG_LOG(7) << "::GetStandablePoint search direction is " << direction;

   while (bounds_.Contains(location)) {
      NG_LOG(7) << "::GetStandablePoint checking location " << location;
      if (q.IsStandable(location)) {
         NG_LOG(7) << "Found it!  Breaking";
         break;
      }
      NG_LOG(7) << "Still not standable.  Searching...";
      location += direction;
   }
   return location;
}

csg::Point3 NavGrid::GetStandablePoint(csg::Point3 const& pt)
{
   csg::Point3 location = bounds_.GetClosestPoint(csg::ToClosestInt(pt));
   csg::Point3 direction(0, IsBlocked(pt) ? 1 : -1, 0);

   while (bounds_.Contains(location)) {
      if (IsStandable(location)) {
         break;
      }
      location += direction;
   }
   return location;
}

float NavGrid::GetMovementSpeedAt(csg::Point3 const& worldPoint)
{
   csg::Point3 index, offset;
   csg::GetChunkIndex<TILE_SIZE>(worldPoint, index, offset);
   return 1.0f + GridTile(index).GetMovementSpeedBonus(offset);
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
core::Guard NavGrid::NotifyTileDirty(std::function<void(csg::Point3 const&)> const& cb) const
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
 * -- NavGrid::UseFastCollisionDetection
 *
 * Return whether any part of the `entity` is solid for the purposes of collision
 * detection.  Non-solid entities can compute their placement and movement
 * information just by looking at the grid, with is much much faster than having
 * to inspect all the trackers to avoid colliding with itself.
 *
 */
bool NavGrid::UseFastCollisionDetection(om::EntityPtr entity) const
{
   if (entity) {
      om::RegionCollisionShapePtr rcs = entity->GetComponent<om::RegionCollisionShape>();
      if (rcs && rcs->GetRegionCollisionType() == om::RegionCollisionShape::SOLID) {
         // can't use the fast collision detection path here because we'll collide with
         // the bits our own shape put into the nav grid tiles!
         return false;
      }
      if (GetMobCollisionType(entity) == om::Mob::TITAN) {
         // can't use the fast collision detection path here because titan's aren't
         // obstructed by small objects.
         return false;
      }
   }
   return true;
}

/*
 * -- GetEntityWorldCollisionShape
 *
 * Get a region representing the collision shape of the `entity` at world location
 * `location`.  This shape is useful ONLY for doing queries about the world (e.g.
 * can an entity of this shape fit here?). 
 *
 */
csg::CollisionShape GetEntityWorldCollisionShape(om::EntityPtr entity, csg::Point3 const& location)
{
   csg::Point3f pos(csg::Point3f::zero);

   if (entity) {
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         if (mob->GetMobCollisionType() != om::Mob::NONE) {
            return mob->GetMobCollisionRegion().Translated(csg::ToFloat(location));
         }
         om::EntityRef entityRoot;
         pos = mob->GetWorldGridLocation(entityRoot);
      }

      om::RegionCollisionShapePtr rcs = entity->GetComponent<om::RegionCollisionShape>();
      if (rcs) {
         om::Region3fBoxedPtr shape = rcs->GetRegion();
         if (shape) {
            switch (rcs->GetRegionCollisionType()) {
            case om::RegionCollisionShape::SOLID: {
               csg::Region3f region = LocalToWorld(shape->Get(), entity);
               csg::Point3f delta = csg::ToFloat(location) - pos;
               if (delta != csg::Point3f::zero) {
                  region.Translate(delta);
               }
               return region;
            }
            default:
               break;
            }
         }
      }
   }
   return csg::CollisionShape();
}

/*
 * -- NavGrid::RemoveEntity
 *
 * Remove the entity from the NavGrid.  This should only be called by the OctTree
 * to notify the NavGrid that the entity in question has been removed from the
 * terrain and should no longer be tracked.
 *
 */
void NavGrid::RemoveEntity(dm::ObjectId id)
{
   auto i = collision_trackers_.find(id);
   if (i != collision_trackers_.end()) {
      for (auto const& entry : i->second) {
         ASSERT(stdutil::contains(collision_tracker_dtors_, entry.first));
         collision_tracker_dtors_.erase(entry.first);
      }
      collision_trackers_.erase(i);
   }
}

bool NavGrid::IsTerrain(csg::Point3 const& location)
{
   csg::Point3 index, offset;
   csg::GetChunkIndex<TILE_SIZE>(location, index, offset);
   bool isTerrain = GridTile(index).IsTerrain(offset);

   NG_LOG(7) << "checking if " << location << " is terrain (result:" << std::boolalpha << isTerrain << ")";
   return isTerrain;
}

void NavGrid::SetRootEntity(om::EntityPtr root)
{
   rootEntity_ = root;
}


bool NavGrid::CanPassThrough(om::EntityPtr const& entity, csg::Point3 const& worldPoint)
{
   csg::Point3 index, offset;
   csg::GetChunkIndex<TILE_SIZE>(worldPoint, index, offset);
   return GridTile(index).CanPassThrough(entity, offset);
}


/*
 * -- NavGrid::CheckoutTrackerVector
 *
 * Return a vector of CollisionTrackerRef's a NavGridTile can use as scratch
 * space.  We keep these around and hand them out to tiles who need them.  The
 * alterative would be to allocate the vectors on the stack, which is very very
 * malloc heavy.
 *
 */

std::vector<CollisionTrackerRef> NavGrid::CheckoutTrackerVector()
{
   tbb::spin_mutex::scoped_lock lock(_trackerVectorLock);
   if (_trackerVectors.empty()) {
      return std::vector<CollisionTrackerRef>();
   }
   std::vector<CollisionTrackerRef> v = std::move(_trackerVectors.front());
   _trackerVectors.pop_front();
   return v;
}


/*
 * -- NavGrid::ReleaseTrackerVector
 *
 * Put brack a tracker checked out with CheckoutTrackerVector so someone
 * else can use it.
 *
 */

void NavGrid::ReleaseTrackerVector(std::vector<CollisionTrackerRef>& trackers)
{
   tbb::spin_mutex::scoped_lock lock(_trackerVectorLock);
   _trackerVectors.emplace_back(std::move(trackers));
}

csg::Region3f NavGrid::ClipRegion(csg::Region3f const& region, ClippingMode mode)
{
   csg::Region3f clippedRegion = region & csg::ToFloat(bounds_);

   ForEachTrackerInShape(region, [&clippedRegion, mode](CollisionTrackerPtr tracker) -> bool {
      bool clip = false;
      TrackerType type = tracker->GetType();
      switch (mode) {
      case CLIP_TERRAIN:
         clip = (type == TERRAIN);
         break;
      case CLIP_SOLID:
         clip = (type == COLLISION || type == TERRAIN);
         break;
      };
      if (clip) {
         tracker->ClipRegion(clippedRegion);
      }
      return false;     // keep going!
   });

   return clippedRegion;
}

csg::Region3f NavGrid::ProjectRegion(csg::Region3f const& region, ClippingMode mode)
{
   csg::Region3 projected;

   // This is the slow, crappy version =(
   for (csg::Cube3f c : csg::EachCube(region)) {
      csg::Cube3 cube = csg::ToInt(c);
      csg::Point3 pt;
      for (pt.z = cube.min.z; pt.z < cube.max.z; pt.z++) {
         for (pt.x = cube.min.x; pt.x < cube.max.x; pt.x++) {
            pt.y = cube.min.y;
            while (bounds_.Contains(pt)) {
               bool blocked;
               if (mode == CLIP_SOLID) {
                  blocked = IsBlocked(pt);
               } else {
                  blocked = IsTerrain(pt);
               }
               if (blocked) {
                  break;
               }
               --pt.y;
            }
            ++pt.y;
            projected.Add(csg::Cube3(pt, csg::Point3(pt.x + 1, cube.max.y, pt.z + 1), cube.GetTag()));
         }
      }
   }
   return csg::ToFloat(projected);
}
