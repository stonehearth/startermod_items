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
 * -- NavGridTileData::IsEmpty
 *
 * Returns true if every point in the cube is not marked in the COLLISION set.
 * Otherwise, false.
 *
 * The cube must be passed in *tile local* coordinates (so 0 - 15 for all
 * coordinates)
 */
bool NavGridTileData::IsEmpty(csg::Cube3 const& cube)
{
   ASSERT((dirty_ & BASE_VECTORS) == 0);

   for (csg::Point3 const& pt : cube) {
      if (IsMarked(COLLISION, pt)) {
         return false;
      }
   }
   return true;
}

/*
 * -- NavGridTileData::CanStandOn
 *
 * Returns true if the can_stand bit is set.  Otherwise, false.  See
 * UpdateCanStand for more info.
 *
 * The pt must be passed in *tile local* coordinates (so 0 - 15 for all
 * coordinates)
 */
bool NavGridTileData::CanStandOn(csg::Point3 const& pt)
{
   ASSERT((dirty_ & DERIVED_VECTORS) == 0);

   return can_stand_[Offset(pt)];
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
void NavGridTileData::FlushDirty(NavGrid& ng, std::vector<CollisionTrackerRef>& trackers, csg::Cube3 const& world_bounds)
{
   UpdateBaseVectors(trackers, world_bounds);
   UpdateDerivedVectors(ng, world_bounds);
   dirty_ = 0;
}

/*
 * -- NavGridTileData::UpdateBaseVectors
 *
 * Update the base vectors for the navgrid.
 */
void NavGridTileData::UpdateBaseVectors(std::vector<CollisionTrackerRef>& trackers, csg::Cube3 const& world_bounds)
{
   if (dirty_ & BASE_VECTORS) {
      dirty_ &= ~BASE_VECTORS;

      for (BitSet& bs : marked_) {
         bs.reset();
      }
      stdutil::ForEachPrune<CollisionTracker>(trackers, [this, &world_bounds](CollisionTrackerPtr tracker) {
         UpdateCollisionTracker(*tracker, world_bounds);
      });
   }
}

/*
 * -- NavGridTileData::UpdateDerivedVectors
 *
 * Update the derived vectors for the navgrid.  Will verify that the base vectors
 * have been updated for you.
 */
void NavGridTileData::UpdateDerivedVectors(NavGrid& ng, csg::Cube3 const& world_bounds)
{
   ASSERT((dirty_ & BASE_VECTORS) == 0);
   if (dirty_ & DERIVED_VECTORS) {
      dirty_ &= ~DERIVED_VECTORS;
      UpdateCanStand(ng, world_bounds);
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

/*
 * -- NavGridTileData::ShowDebugShapes
 *
 * Draw some debug shapes.  We draw a frame around the entire tile, then a blue
 * filter over the places where someone can stand.
 */
void NavGridTileData::ShowDebugShapes(protocol::shapelist* msg, csg::Cube3 const& world_bounds)
{
   ASSERT(dirty_ == 0);

   NG_LOG(5) << "sending debug shapes for tile ";

   csg::Color4 blocked_color(0, 0, 255, 64);
   csg::Color4 border_color(0, 0, 128, 64);

   csg::Cube3 bounds(csg::Point3::zero, csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));
   for (csg::Point3 const& i : bounds) {
      if (can_stand_[Offset(i)]) {
         csg::Point3 coord_pt = world_bounds.min + i;
         protocol::coord* coord = msg->add_coords();
         coord_pt.SaveValue(coord);
         blocked_color.SaveValue(coord->mutable_color());
      }
   }

   protocol::box* box = msg->add_box();
   csg::ToFloat(world_bounds.min).SaveValue(box->mutable_minimum());
   csg::ToFloat(world_bounds.max).SaveValue(box->mutable_maximum());
   border_color.SaveValue(box->mutable_color());
}

/*
 * -- NavGridTileData::UpdateCanStand
 *
 * Update the derived _can_stand vector.  We define areas that are valid for standing
 * as ones that are supported by a LADDER or COLLISION bit and have CHARACTER_HEIGHT
 * voxels above then that are "air", where "air" means !COLLISION || LADDER.
 */
void NavGridTileData::UpdateCanStand(NavGrid& ng, csg::Cube3 const& world_bounds)
{
   int i, count = 0;
   static int CHARACTER_HEIGHT = 4;
   csg::Point3 index = csg::GetChunkIndex(world_bounds.min, TILE_SIZE);

   csg::Point3 above_pt = index + csg::Point3(0, 1, 0);
   NavGridTile& above = ng.GridTileResident(above_pt);
   above.UpdateBaseVectors(above_pt);
   std::shared_ptr<NavGridTileData> above_data = above.GetTileData();

   csg::Point3 below_pt = index - csg::Point3(0, 1, 0);
   NavGridTile& below = ng.GridTileResident(below_pt);
   below.UpdateBaseVectors(below_pt);
   std::shared_ptr<NavGridTileData> below_data = below.GetTileData();

   csg::Cube3 bounds(csg::Point3::zero, csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));

   // xxx: this loop could be made CHARACTER_HEIGHT times faster with a little more
   // elbow grease...
   can_stand_.reset();
   for (csg::Point3 pt : bounds) {
      NG_LOG(9) << "considering can_stand point " << pt;

      // Check the point directly below this one to see if it's in the LADDER or
      // COLLISION vectors.  That means we can stand there if there's enough room
      // overhead
      bool bottom_edge = pt.y == 0;
      NavGridTileData* tile = !bottom_edge ? this : below_data.get();
      csg::Point3 query_pt = !bottom_edge ? (pt - csg::Point3(0, 1, 0)) : csg::Point3(pt.x, TILE_SIZE-1, pt.z);

      int base = Offset(query_pt);
      if (tile->IsMarked(COLLISION, base) || tile->IsMarked(LADDER, base)) {
         // Compute how much room is overhead by making sure none of the COLLISION bits
         // are set for CHARACTER_HEIGHT voxels above this one (or the LADDER bit
         // is set).
         tile = this;
         csg::Point3 riser = pt;
         for (i = 0; i < CHARACTER_HEIGHT; i++) {
            if (riser.y == TILE_SIZE) {
               riser.y = 0;
               tile = above_data.get();
            }
            bool empty_enough = !tile->IsMarked(COLLISION, riser) || tile->IsMarked(LADDER, riser);
            if (!empty_enough) {
               break;
            }
            riser.y++;
         }
         if (i == CHARACTER_HEIGHT) {
            NG_LOG(7) << "adding " << (world_bounds.min + pt) << " to navgrid (offset" << pt << ")";
            can_stand_[Offset(pt)] = true;
            count++;
         }
      }
   }
   NG_LOG(5) << "marked " << count << " bits in can_stand vector.";
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

