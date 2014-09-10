#include "radiant.h"
#include "radiant_stdutil.h"
#include "csg/cube.h"
#include "csg/region.h"
#include "collision_tracker.h"
#include "om/entity.h"
#include "nav_grid.h"
#include "nav_grid_tile.h"
#include "nav_grid_tile_data.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG(physics.navgrid, level)

/* 
 * -- NavGridTile::NavGridTile
 *
 * Construct a new NavGridTile.
 *
 */
NavGridTile::NavGridTile(NavGrid& ng, csg::Point3 const& index) :
   _ng(ng),
   _index(index),
   _residentTileIndex(-1),
   changed_slot_("tile changes")
{
}

/*
 * -- NavGridTile::RemoveCollisionTracker
 *
 * Remove a collision tracker from the appropriate list.  Just set the dirty bit for now.
 *
 */
void NavGridTile::RemoveCollisionTracker(CollisionTrackerPtr tracker)
{
   om::EntityPtr entity = tracker->GetEntity();
   if (entity) {
      dm::ObjectId entityId = entity->GetObjectId();

      // Run through all the trackers for the current entity.  The TrackerMap is a
      // multimap, so this is O(1) for the find() + O(n) for the number of tracker
      // types, but there are a only and handful of those, so consider the while
      // loop here to be constant time.

      auto range = trackers_.equal_range(entityId);
      for (auto i = range.first; i != range.second; i++) {
         if (i->second.lock() == tracker) {
            trackers_.erase(i);
            OnTrackerRemoved(entityId, tracker->GetType());
            return;
         }
      }
   }
}

/*
 * -- NavGridTile::AddCollisionTracker
 *
 * Add a collision tracker to the appropriate list.  Just set the dirty bit for now.
 *
 */
void NavGridTile::AddCollisionTracker(CollisionTrackerPtr tracker)
{
   om::EntityPtr entity = tracker->GetEntity();
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      TrackerMap::const_iterator i;
      auto range = trackers_.equal_range(id);
      for (i = range.first; i != range.second; ++i) {
         if (i->second.lock() == tracker) {
            break;
         }
      }
      if (i == range.second) {
         trackers_.insert(std::make_pair(id, tracker));
         changed_slot_.Signal(ChangeNotification(ENTITY_ADDED, id, tracker));
      } else {
         changed_slot_.Signal(ChangeNotification(ENTITY_MOVED, id, tracker));         
      }
      MarkDirty(tracker->GetType());
   }
}

/*
 * -- NavGridTile::IsBlocked
 *
 * Returns true if any point in the cube is marked in the COLLISION set.
 * Otherwise, false.
 *
 * `cube` must be passed in *tile local* coordinates (so 0 - 15 for all
 * coordinates)
 */

bool NavGridTile::IsBlocked(csg::Point3 const& pt)
{
   ASSERT(data_);
   ASSERT(csg::Cube3::one.Scaled(TILE_SIZE).Contains(pt));

   return data_->IsMarked(COLLISION, pt);
}

bool NavGridTile::IsBlocked(csg::Cube3 const& cube)
{
   return IsMarked(&NavGridTile::IsBlocked, cube);
}

bool NavGridTile::IsBlocked(csg::Region3 const& region)
{
   return IsMarked(&NavGridTile::IsBlocked, region);
}


/*
 * -- NavGridTile::IsSupport
 *
 * Returns true if the can_stand `pt` is a support point.
 *
 * The pt must be passed in *tile local* coordinates (so 0 - 15 for all
 * coordinates)
 *
 */
bool NavGridTile::IsSupport(csg::Point3 const& pt)
{
   ASSERT(data_);
   ASSERT(csg::Cube3::one.Scaled(TILE_SIZE).Contains(pt));

   return data_->IsMarked(COLLISION, pt) || data_->IsMarked(LADDER, pt);
}

bool NavGridTile::IsSupport(csg::Cube3 const& cube)
{
   return IsMarked(&NavGridTile::IsSupport, cube);
}

bool NavGridTile::IsSupport(csg::Region3 const& region)
{
   return IsMarked(&NavGridTile::IsSupport, region);
}


/*
 * -- NavGridTile::IsTerrain
 *
 * Returns true if the can_stand `pt` is a support point.
 *
 * The pt must be passed in *tile local* coordinates (so 0 - 15 for all
 * coordinates)
 *
 */
bool NavGridTile::IsTerrain(csg::Point3 const& pt)
{
   ASSERT(data_);
   ASSERT(csg::Cube3::one.Scaled(TILE_SIZE).Contains(pt));

   return data_->IsMarked(TERRAIN, pt);
}

bool NavGridTile::IsTerrain(csg::Cube3 const& cube)
{
   return IsMarked(&NavGridTile::IsTerrain, cube);
}

bool NavGridTile::IsTerrain(csg::Region3 const& region)
{
   return IsMarked(&NavGridTile::IsTerrain, region);
}

bool NavGridTile::IsMarked(IsMarkedPredicate predicate, csg::Region3 const& region)
{
   for (csg::Cube3 const& cube : region ) {
      if (!IsMarked(predicate, cube)) {
         return false;
      }
   }
   return true;
}

bool NavGridTile::IsMarked(IsMarkedPredicate predicate, csg::Cube3 const& cube)
{
   ASSERT(data_);
   ASSERT(csg::Cube3::one.Scaled(TILE_SIZE).Contains(cube.min));
   ASSERT(csg::Cube3::one.Scaled(TILE_SIZE+1).Contains(cube.max));

   for (csg::Point3 pt : cube) {
      if ((this->*predicate)(pt)) {
         return true;
      }
   }
   return false;
}

/*
 * -- NavGridTile::FlushDirty
 *
 * Re-generates all the tile data.  We don't bother keeping track of what's actually
 * changed.. just start over from scratch.  The bookkeeping for "proper" change tracking
 * is more complicated and expensive relative to the cost of generating the bit vector,
 * so just don't bother.  This updates each of the individual TrackerType vectors first,
 * then walk through the derived vectors.
 *
 */
void NavGridTile::FlushDirty(NavGrid& ng)
{
   ASSERT(data_);

   data_->FlushDirty(ng, trackers_, GetWorldBounds());
}

/*
 * -- NavGridTile::SetDataResident
 *
 * Load or unload the NavGridTileData for this tile.  In the load case, this only
 * creates the Data object.  It will be updated lazily when required.
 *
 */
void NavGridTile::SetDataResident(bool value, int index)
{
   if (value && !data_) {
      data_.reset(new NavGridTileData());
   } else if (!value && data_) {
      data_.reset(nullptr);
   }
   _residentTileIndex = index;

   if (data_.get()) {
      ASSERT(_residentTileIndex >= 0);
   } else {
      ASSERT(_residentTileIndex < 0);
   }
}
 


/*
 * -- NavGridTile::MarkDirty
 *
 * Mark the tile dirty.
 *
 */
void NavGridTile::MarkDirty(TrackerType t)
{
   if (t < NUM_BIT_VECTOR_TRACKERS) {
      NG_LOG(7) << "marking grid tile " << _index << " as dirty (tracker type:" << t << ")";
      _ng.SignalTileDirty(_index);
      if (data_) {
         data_->MarkDirty();
      }
   } else  {
      NG_LOG(7) << "not-marking grid tile " << _index << " as dirty (tracker type:" << t << " not a collision type)";
   }
}

/*
 * -- NavGridTile::OnTrackerRemoved
 *
 * Called whenever any tracker for `entityId` gets removed from the set.
 *
 */
void NavGridTile::OnTrackerRemoved(dm::ObjectId entityId, TrackerType t)
{
   MarkDirty(t);
   changed_slot_.Signal(ChangeNotification(ENTITY_REMOVED, entityId, nullptr));
}

/*
 * -- NavGridTile::GetWorldBounds
 *
 * Given the address of the tile in the world, compute its bounds.
 *
 */
csg::Cube3 NavGridTile::GetWorldBounds() const
{
   csg::Point3 min(_index.Scaled(TILE_SIZE));
   return csg::Cube3(min, min + csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));
}

/*
 * -- NavGridTile::ForEachTracker
 *
 * Call the `cb` for all trackers which overlap the tile.  This function makes
 * no guarantees as to the number of times the cb will get called per entity!
 * Since `trackers_` is a multimap, the cb will be called for each tracker
 * type registered by the entity.  We could filter out duplicates but (1)
 * that's expensive and redundant if the caller is also filtering dups and
 * (2) most callers will already have to handle redundant calls when 
 * iterating over a range of tiles for cases when the same tracker overlaps
 * several tiles.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */ 
bool NavGridTile::ForEachTracker(ForEachTrackerCb const& cb)
{
   bool stopped = ForEachTrackerInRange(trackers_.begin(), trackers_.end(), cb);
   return stopped;
}

/*
 * -- NavGridTile::ForEachTrackerForEntity
 *
 * Call the `cb` for all trackers for the specified `entityId`.  Stops iteration when
 * the `cb` returns false.  Returns whether or not we made it through the whole
 * list.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */ 
bool NavGridTile::ForEachTrackerForEntity(dm::ObjectId entityId, ForEachTrackerCb const& cb)
{
   auto range = trackers_.equal_range(entityId);
   bool stopped = ForEachTrackerInRange(range.first, range.second, cb);
   return stopped;
}

/*
 * -- NavGridTile::ForEachTrackerInRange
 *
 * Call the `cb` for all trackers in the specified range.  Stops iteration when
 * the `cb` returns false.  Returns whether or not we made it through the whole
 * list.
 *
 * Stops iteration whenever a cb returns 'true'.  Itself returns 'true' if the iteration
 * was stopped early.
 *
 */ 
bool NavGridTile::ForEachTrackerInRange(TrackerMap::const_iterator begin, TrackerMap::const_iterator end, ForEachTrackerCb const& cb)
{
   int tempSize = tempTrackers_.size();
   int numTrackers = 0;
   bool stopped = false;

   // It's important here not to modify the trackers array at all during iterator.
   // Otherwise, we'll invalidate the `end` iterator and certainly blow up somewhere!
   // Also: we never call 'clear' on tempTrackers_, because it's only purpose is to hold
   // a list of weak refs to the actual trackers that we can safely iterate over; no
   // need to call destroy on that list (since they're weak refs).
   while (begin != end && numTrackers < tempSize) {
      tempTrackers_[numTrackers++] = begin->second;
      ++begin;
   }
   while (begin != end) {
      tempTrackers_.emplace_back(begin->second);
      numTrackers++;
      ++begin;
   }

   for (int i = 0; i < numTrackers; i++) {
      CollisionTrackerPtr tracker = tempTrackers_[i].lock();
      if (tracker) {
         om::EntityPtr entity = tracker->GetEntity();
         if (entity) {
            NG_LOG(7) << "calling ForEachTracker callback on " << *entity;
            stopped = cb(tracker);
            if (stopped) {
               break;
            }
         }
      }
   }
   return stopped;
}

/*
 * -- NavGridTile::RegisterChangeCb
 *
 * Calls the `cb` whenever the state of the NavGridTile changes.  See 
 * NavGridTile::ChangeNotification for more details.
 *
 */ 
core::Guard NavGridTile::RegisterChangeCb(ChangeCb const& cb)
{
   return changed_slot_.Register(cb);
}


/*
 * -- NavGridTile::GetResidentTileIndex
 *
 * Returns the index in the nav_grid's resident tile array where this
 * tile is stored.  This is necessary to help support the O(1) update
 * of the cache array in NavGrid::GridTile.
 *
 */ 
int NavGridTile::GetResidentTileIndex() const
{
   return _residentTileIndex;
}
