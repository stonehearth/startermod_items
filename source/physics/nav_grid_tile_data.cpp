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
bool NavGridTileData::IsMarked(TrackerType type, csg::Point3 const& offset)
{
   return IsMarked(type, Offset(offset.x, offset.y, offset.z));
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
inline int NavGridTileData::Offset(int x, int y, int z)
{
   DEBUG_ONLY(
      ASSERT(x >= 0 && y >= 0 && z >= 0);
      ASSERT(x < TILE_SIZE && y < TILE_SIZE && z < TILE_SIZE);
      )
   return TILE_SIZE * (y + (TILE_SIZE * x)) + z;
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

      if (type == PLATFORM) {
         type = COLLISION;
      }

      // This code is on the critical path when pathing during construction; so, we avoid using Point3
      // iterators, and duplicate the loops in order to avoid an interior branch.
      if (type == TERRAIN) {
         auto& marked_terrain = marked_[TERRAIN];
         auto& marked_collision = marked_[COLLISION];
         for (csg::Cube3 const& cube : overlap) {
            for (int y = cube.GetMin().y; y < cube.GetMax().y; y++) {
               for (int x = cube.GetMin().x; x < cube.GetMax().x; x++) {
                  for (int z = cube.GetMin().z; z < cube.GetMax().z; z++) {
                     int offset = Offset(x, y, z);

                     DEBUG_ONLY(
                        NG_LOG(9) << "marking (" << x << ", " << y << ", " << z << ") in vector " << type;
                     )
                     marked_terrain.set(offset);
                     marked_collision.set(offset);
                  }
               }
            }
         }
      } else {
         auto& marked_type = marked_[type];
         for (csg::Cube3 const& cube : overlap) {
            for (int y = cube.GetMin().y; y < cube.GetMax().y; y++) {
               for (int x = cube.GetMin().x; x < cube.GetMax().x; x++) {
                  for (int z = cube.GetMin().z; z < cube.GetMax().z; z++) {
                     int offset = Offset(x, y, z);

                     DEBUG_ONLY(
                        NG_LOG(9) << "marking (" << x << ", " << y << ", " << z << ") in vector " << type;
                     )
                     marked_type.set(offset);
                  }
               }
            }
         }
      }
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

