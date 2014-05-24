#include "radiant.h"
#include "radiant_stdutil.h"
#include "csg/region.h"
#include "csg/color.h"
#include "csg/util.h"
#include "nav_grid.h"
#include "nav_grid_tile_data.h"
#include "collision_tracker.h"
#include "protocols/radiant.pb.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG_CATEGORY(physics.navgrid, level, "physics.navgridtile " << world_bounds.min)

/* 
 * -- NavGridTileData::NavGridTileData
 *
 * Construct a new NavGridTileData.
 */
NavGridTileData::NavGridTileData() :
   dirty_(ALL_DIRTY_BITS)
{
}

NavGridTileData::~NavGridTileData()
{
}

/*
 * -- NavGridTileData::IsMarked
 *
 * Checks to see if the point for the specified tracker is set.
 */
bool NavGridTileData::IsMarked(TrackerType type, csg::Point3 const& offest)
{
   return IsMarked(type, Offset(offest));
}

/*
 * -- NavGridTileData::IsMarked
 *
 * Like the point version of IsMarked, but accepts an offset into the
 * bitvector rather than a point address.
 */
bool NavGridTileData::IsMarked(TrackerType type, int bit_index)
{
   ASSERT(bit_index >= 0 && bit_index < TILE_SIZE * TILE_SIZE * TILE_SIZE);
   return marked_[type][bit_index];
}

/*
 * -- NavGridTileData::Offset
 *
 * Converts a point address inside the tile to an offset to the bit.
 */
int NavGridTileData::Offset(csg::Point3 const& pt)
{
   ASSERT(pt.x >= 0 && pt.y >= 0 && pt.z >= 0);
   ASSERT(pt.x < TILE_SIZE && pt.y < TILE_SIZE && pt.z < TILE_SIZE);
   return TILE_SIZE * (pt.z + (TILE_SIZE * pt.y)) + pt.x;
}

/*
 * -- NavGridTileData::FlushDirty
 *
 * Re-generates all the tile data.  We don't bother keeping track of what's actually
 * changed.. just start over from scratch.  The bookkeeping for "proper" change tracking
 * is more complicated and expensive relative to the cost of generating the bitvector,
 * so just don't bother.  This updates each of the individual TrackerType vectors first,
 * then walk through the derived vectors.
 */
void NavGridTileData::FlushDirty(NavGrid& ng, TrackerMap& trackers, csg::Cube3 const& world_bounds)
{
   UpdateBaseVectors(trackers, world_bounds);
   dirty_ = 0;
}

/*
 * -- NavGridTileData::UpdateBaseVectors
 *
 * Update the base vectors for the navgrid.
 */
void NavGridTileData::UpdateBaseVectors(TrackerMap& trackers, csg::Cube3 const& world_bounds)
{
   if (dirty_ & BASE_VECTORS) {
      dirty_ &= ~BASE_VECTORS;

      for (BitSet& bs : marked_) {
         bs.reset();
      }
      stdutil::ForEachPrune<dm::ObjectId, CollisionTracker>(trackers, [this, &world_bounds](dm::ObjectId const&, CollisionTrackerPtr tracker) {
         UpdateCollisionTracker(*tracker, world_bounds);
      });
   }
}

/*
 * -- NavGridTileData::UpdateCollisionTracker
 *
 * Ask the tracker to compute the region which overlaps this tile, and add every point
 * in that region to the bitvector.
 */
void NavGridTileData::UpdateCollisionTracker(CollisionTracker const& tracker, csg::Cube3 const& world_bounds)
{
   // Ask the tracker to compute the overlap with this tile.  world_bounds is stored in
   // world-coordinates, so translate it back to tile-coordinates when we get it back.
   TrackerType type = tracker.GetType();
   if (type < NUM_BIT_VECTOR_TRACKERS) {
      csg::Region3 overlap = tracker.GetOverlappingRegion(world_bounds).Translated(-world_bounds.min);

      // Now just loop through all the points and set the bit.  There's certainly a more
      // efficient implementation which sets contiguous bits in one go, but this already
      // doesn't appear on the profile (like < 0.1% of the total game time)
      int count = 0;
      for (csg::Cube3 const& cube : overlap) {
         for (csg::Point3 const& pt : cube) {
            NG_LOG(9) << "marking " << pt << " in vector " << type;
            marked_[type][Offset(pt)] = true;
            count++;
         }
      }
      NG_LOG(5) << "marked " << count << " bits in vector " << type;
   }
}


/*
 * -- NavGridTileData::MarkDirty
 *
 * Mark the tile dirty.
 */
void NavGridTileData::MarkDirty()
{
   dirty_ = ALL_DIRTY_BITS;
}

