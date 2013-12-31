#include "radiant.h"
#include "radiant_stdutil.h"
#include "csg/cube.h"
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
   stdutil::FastRemove(trackers_, tracker);
   MarkDirty();
}

/*
 * -- NavGridTile::AddCollisionTracker
 *
 * Add a collision tracker to the appropriate list.  Just set the dirty bit for now.
 */
void NavGridTile::AddCollisionTracker(CollisionTrackerPtr tracker)
{
   stdutil::UniqueInsert(trackers_, tracker);
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
 * is more complicated and expensive relative to the cost of generating the bitvector,
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
