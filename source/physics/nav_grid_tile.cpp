#include "radiant.h"
#include "radiant_stdutil.h"
#include "csg/cube.h"
#include "collision_tracker.h"
#include "om/entity.h"
#include "nav_grid_tile.h"
#include "nav_grid_tile_data.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG_CATEGORY(physics.navgrid, level, "physics.navgridtile " << index_)

/* 
 * -- NavGridTile::NavGridTile
 *
 * Construct a new NavGridTile.
 */
NavGridTile::NavGridTile() :
   visited_(false)
{
}

/*
 * -- NavGridTile::RemoveCollisionTracker
 *
 * Remove a collision tracker from the appropriate list.  Just set the dirty bit for now.
 */
void NavGridTile::RemoveCollisionTracker(CollisionTrackerPtr tracker)
{
   om::EntityPtr entity = tracker->GetEntity();
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();

      // Run through all the trackers for the current entity.  The TrackerMap is a
      // multimap, so this is O(1) for the find() + O(n) for the number of tracker
      // types, but there are a only and handful of those, so consider the while
      // loop here to be constant time.
      auto range = trackers_.equal_range(id);
      for (auto i = range.first; i != range.second; i++) {
         if (i->second.lock() == tracker) {
            trackers_.erase(i);
            MarkDirty();
            return;
         }
      }
   }
}

/*
 * -- NavGridTile::AddCollisionTracker
 *
 * Add a collision tracker to the appropriate list.  Just set the dirty bit for now.
 */
void NavGridTile::AddCollisionTracker(CollisionTrackerPtr tracker)
{
   om::EntityPtr entity = tracker->GetEntity();
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      trackers_.insert(std::make_pair(id, tracker));
      MarkDirty();
   }
   MarkDirty();
}

/*
 * -- NavGridTile::IsEmpty
 *
 * Returns true if every point in the cube is not marked in the COLLISION set.
 * Otherwise, false.
 *
 * The cube must be passed in *tile local* coordinates (so 0 - 15 for all
 * coordinates)
 */
bool NavGridTile::IsEmpty(csg::Cube3 const& cube)
{
   ASSERT(data_);
   return data_->IsEmpty(cube);
}

/*
 * -- NavGridTile::CanStandOn
 *
 * Returns true if the can_stand bit is set.  Otherwise, false.  See
 * UpdateCanStand for more info.
 *
 * The pt must be passed in *tile local* coordinates (so 0 - 15 for all
 * coordinates)
 */
bool NavGridTile::CanStandOn(csg::Point3 const& pt)
{
   ASSERT(data_);
   return data_->CanStandOn(pt);
}

/*
 * -- NavGridTile::FlushDirty
 *
 * Re-generates all the tile data.  We don't bother keeping track of what's actually
 * changed.. just start over from scratch.  The bookkeeping for "proper" change tracking
 * is more complicated and expensive relative to the cost of generating the bit vector,
 * so just don't bother.  This updates each of the individual TrackerType vectors first,
 * then walk through the derived vectors.
 */
void NavGridTile::FlushDirty(NavGrid& ng, csg::Point3 const& index)
{
   ASSERT(data_);
   data_->FlushDirty(ng, trackers_, GetWorldBounds(index));
}

/*
 * -- NavGridTile::UpdateDerivedVectors
 *
 * Update the base vectors for the navgrid.
 */
void NavGridTile::UpdateBaseVectors(csg::Point3 const& index)
{
   ASSERT(data_);
   data_->UpdateBaseVectors(trackers_, GetWorldBounds(index));
}

/*
 * -- NavGridTile::ShowDebugShapes
 *
 * Draw some debug shapes.
 */
void NavGridTile::ShowDebugShapes(protocol::shapelist* msg, csg::Point3 const& index)
{
   ASSERT(data_);
   data_->ShowDebugShapes(msg, GetWorldBounds(index));
}

/*
 * -- NavGridTile::IsDataResident
 *
 * Return whether or not the NavGridTileData for this tile is loaded.
 */
bool NavGridTile::IsDataResident() const
{
   return data_.get() != nullptr;
}

/*
 * -- NavGridTile::SetDataResident
 *
 * Load or unload the NavGridTileData for this tile.  In the load case, this only
 * creates the Data object.  It will be updated lazily when required.
 */
void NavGridTile::SetDataResident(bool value)
{
   if (value && !data_) {
      data_ = std::make_shared<NavGridTileData>();
   } else if (!value && data_) {
      data_ = nullptr;
   }
}


/*
 * -- NavGridTile::MarkDirty
 *
 * Mark the tile dirty.
 */
void NavGridTile::MarkDirty()
{
   if (data_) {
      data_->MarkDirty();
   }
}


/*
 * -- NavGridTile::GetWorldBounds
 *
 * Given the address of the tile in the world, compute its bounds.
 */
csg::Cube3 NavGridTile::GetWorldBounds(csg::Point3 const& index) const
{
   csg::Point3 min(index.Scaled(TILE_SIZE));
   return csg::Cube3(min, min + csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));
}

/*
 * -- NavGridTile::GetTileData
 *
 * Get the NavGridTileData for this tile.  This is only used by other
 * NavGridTileData objects to handle computation on the borders.
 */
std::shared_ptr<NavGridTileData> NavGridTile::GetTileData()
{
   return data_;
}

/*
 * -- NavGridTile::ForEachEntity
 *
 * Call the `cb` for all entities which overlap the tile.  This function makes
 * no guarantees as to the number of times the cb will get called per entity!
 * Since `trackers_` is a multimap, the cb will be called for each tracker
 * type registered by the entity.  We could filter out duplicates but (1)
 * that's expensive and redundant if the caller is also filtering dups and
 * (2) most callers will already have to handle redundant calls when 
 * iterating over a range of tiles for cases when the same entity overlaps
 * several tiles.
 */ 
void NavGridTile::ForEachEntity(ForEachEntityCb cb)
{
   stdutil::ForEachPrune<dm::ObjectId, CollisionTracker>(trackers_, [cb](dm::ObjectId const& id, CollisionTrackerPtr tracker) {
      cb(tracker->GetEntity());
   });
}
