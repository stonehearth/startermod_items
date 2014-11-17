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
NavGridTileData::NavGridTileData(NavGridTile& ngt) :
   _ngt(ngt),
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
template <TrackerType Type>
bool NavGridTileData::IsMarked(csg::Point3 const& offset)
{
   int bitIndex = Offset(offset.x, offset.y, offset.z);

   ASSERT(bitIndex >= 0 && bitIndex < TILE_SIZE * TILE_SIZE * TILE_SIZE);
   UpdateTileData<Type>();
   return marked_[Type][bitIndex];
}
template <TrackerType DstType>
void NavGridTileData::UpdateTileData()
{
   const int SrcMask = (int)(1 << DstType);
   UpdateTileDataForTrackers<SrcMask, DstType>();
}

template <>
void NavGridTileData::UpdateTileData<COLLISION>()
{
   const int SrcMask = (1 << COLLISION) | (1 << TERRAIN);
   UpdateTileDataForTrackers<SrcMask, COLLISION>();
}

template <int SrcMask, TrackerType DstType>
void NavGridTileData::UpdateTileDataForTrackers()
{
   int dirtyMask = (1 << DstType);
   if ((dirty_ & dirtyMask) == 0) {
      return;
   }

   BitSet& bits = marked_[DstType];
   bits.reset();

   csg::Cube3 worldBounds = _ngt.GetWorldBounds();
   _ngt.ForEachTracker([this, &worldBounds, &bits](CollisionTrackerPtr tracker) {
      int mask = (1 << tracker->GetType());

      if ((SrcMask & mask) == 0) {
         return false;     // keep going!
      }
      csg::Region3 overlap = tracker->GetOverlappingRegion(worldBounds).Translated(-worldBounds.min);

      // This code is on the critical path when pathing during construction; so, we avoid using Point3
      // iterators, and duplicate the loops in order to avoid an interior branch.
      for (csg::Cube3 const& cube : EachCube(overlap)) {
         for (int y = cube.GetMin().y; y < cube.GetMax().y; y++) {
            for (int x = cube.GetMin().x; x < cube.GetMax().x; x++) {
               for (int z = cube.GetMin().z; z < cube.GetMax().z; z++) {
                  int offset = Offset(x, y, z);

                  DEBUG_ONLY(
                     NG_LOG(9) << "marking (" << x << ", " << y << ", " << z << ") in vector " << type;
                  )
                  bits.set(offset);
               }
            }
         }
      }
      return false;     // keep going!
   });

   dirty_ &= ~dirtyMask;
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
 * -- NavGridTileData::MarkDirty
 *
 * Mark the tile dirty.
 */
void NavGridTileData::MarkDirty(TrackerType t)
{
   dirty_ |= (1 << t);
   if (t == TERRAIN) {
      dirty_ |= (1 << COLLISION);
   }
}

template bool NavGridTileData::IsMarked<COLLISION>(csg::Point3 const&);
template bool NavGridTileData::IsMarked<TERRAIN>(csg::Point3 const&);
template bool NavGridTileData::IsMarked<LADDER>(csg::Point3 const&);
