#include "radiant.h"
#include "radiant_stdutil.h"
#include "csg/region.h"
#include "csg/color.h"
#include "csg/util.h"
#include "nav_grid.h"
#include "nav_grid_tile.h"
#include "collision_tracker.h"
#include "protocols/radiant.pb.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG_CATEGORY(physics.navgrid, level, "physics.navgridtile " << index_)

/* 
 * -- NavGridTile::NavGridTile
 *
 * Construct a new NavGridTile.  The index is the index used to store the tile, which
 * is simply the position of the min position of the tile / the TILE_SIZE (so the tile
 * rooted at (32, 64, -16) has index (1, 2, -1).
 */
NavGridTile::NavGridTile(NavGrid& ng, csg::Point3 const& index) :
   ng_(ng),
   index_(index),
   dirty_(ALL_DIRTY_BITS)
{
   bounds_.min = index.Scaled(TILE_SIZE);
   bounds_.max = bounds_.min + csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

/*
 * -- NavGridTile::RemoveCollisionTracker
 *
 * Remove a collision tracker from the appropriate list.  Just set the dirty bit for now.
 */
void NavGridTile::RemoveCollisionTracker(TrackerType type, CollisionTrackerPtr tracker)
{
   dirty_ = ALL_DIRTY_BITS;
   stdutil::FastRemove(trackers_[type], tracker);
}

/*
 * -- NavGridTile::AddCollisionTracker
 *
 * Add a collision tracker to the appropriate list.  Just set the dirty bit for now.
 */
void NavGridTile::AddCollisionTracker(TrackerType type, CollisionTrackerPtr tracker)
{
   dirty_ = ALL_DIRTY_BITS;
   stdutil::UniqueInsert(trackers_[type], tracker);
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
   FlushDirty();
   for (csg::Point3 const& pt : cube) {
      if (IsMarked(COLLISION, pt)) {
         return false;
      }
   }
   return true;
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
   FlushDirty();
   return can_stand_[Offset(pt)];
}


/*
 * -- NavGridTile::IsMarked
 *
 * Checks to see if the point for the specified tracker is set.
 */
bool NavGridTile::IsMarked(TrackerType type, csg::Point3 const& offest)
{
   return IsMarked(type, Offset(offest));
}

/*
 * -- NavGridTile::IsMarked
 *
 * Like the point version of IsMarked, but accepts an offset into the
 * bitvector rather than a point address.
 */
bool NavGridTile::IsMarked(TrackerType type, int bit_index)
{
   ASSERT(bit_index >= 0 && bit_index < TILE_SIZE * TILE_SIZE * TILE_SIZE);
   return marked_[type][bit_index];
}

/*
 * -- NavGridTile::Offset
 *
 * Converts a point address inside the tile to an offset to the bit.
 */
int NavGridTile::Offset(csg::Point3 const& pt)
{
   ASSERT(pt.x >= 0 && pt.y >= 0 && pt.z >= 0);
   ASSERT(pt.x < TILE_SIZE && pt.y < TILE_SIZE && pt.z < TILE_SIZE);
   return TILE_SIZE * (pt.z + (TILE_SIZE * pt.y)) + pt.x;
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
void NavGridTile::FlushDirty()
{
   UpdateBaseVectors();
   UpdateDerivedVectors();
   dirty_ = 0;
}

/*
 * -- NavGridTile::UpdateDerivedVectors
 *
 * Update the base vectors for the navgrid.
 */
void NavGridTile::UpdateBaseVectors()
{
   if (dirty_ & BASE_VECTORS) {
      dirty_ &= ~BASE_VECTORS;

      for (int i = 0; i < MAX_TRACKER_TYPES; i++) {
         // Clear all the bits, then set the ones which are contained by
         // the trackers for that type.
         TrackerType type = (TrackerType)i;
         marked_[type].reset();
         stdutil::ForEachPrune<CollisionTracker>(trackers_[type], [type, this](CollisionTrackerPtr tracker) {
            UpdateCollisionTracker(type, *tracker);
         });
      }
   }
}

/*
 * -- NavGridTile::UpdateDerivedVectors
 *
 * Update the derived vectors for the navgrid.  Will verify that the base vectors
 * have been updated for you.
 */
void NavGridTile::UpdateDerivedVectors()
{
   UpdateBaseVectors();
   if (dirty_ & DERIVED_VECTORS) {
      dirty_ &= ~DERIVED_VECTORS;
      UpdateCanStand();
   }
}

/*
 * -- NavGridTile::UpdateCollisionTracker
 *
 * Ask the tracker to compute the region which overlaps this tile, and add every point
 * in that region to the bitvector.
 */
void NavGridTile::UpdateCollisionTracker(TrackerType type, CollisionTracker const& tracker)
{
   // Ask the tracker to compute the overlap with this tile.  bounds_ is stored in
// world-coordinates, so translate it back to tile-coordinates when we get it back.
   csg::Region3 overlap = tracker.GetOverlappingRegion(bounds_).Translated(-bounds_.min);

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
 * -- NavGridTile::ShowDebugShapes
 *
 * Draw some debug shapes.  We draw a frame around the entire tile, then a blue
 * filter over the places where someone can stand.
 */
void NavGridTile::ShowDebugShapes(protocol::shapelist* msg)
{
   NG_LOG(5) << "sending debug shapes for tile " << index_;

   csg::Color4 blocked_color(0, 0, 255, 64);
   csg::Color4 border_color(0, 0, 128, 64);

   // xxx: add a config option for this
   FlushDirty();

   csg::Cube3 bounds(csg::Point3::zero, csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));
   for (csg::Point3 const& i : bounds) {
      if (can_stand_[Offset(i)]) {
         csg::Point3 coord_pt = bounds_.min + i;
         protocol::coord* coord = msg->add_coords();
         coord_pt.SaveValue(coord);
         blocked_color.SaveValue(coord->mutable_color());
      }
   }

   protocol::box* box = msg->add_box();
   csg::ToFloat(bounds_.min).SaveValue(box->mutable_minimum());
   csg::ToFloat(bounds_.max).SaveValue(box->mutable_maximum());
   border_color.SaveValue(box->mutable_color());
}

/*
 * -- NavGridTile::UpdateCanStand
 *
 * Update the derived _can_stand vector.  We define areas that are valid for standing
 * as ones that are supported by a LADDER or COLLISION bit and have CHARACTER_HEIGHT
 * voxels above then that are "air", where "air" means !COLLISION || LADDER.
 */
void NavGridTile::UpdateCanStand()
{
   int i, count = 0;
   static int CHARACTER_HEIGHT = 4;
   NavGridTile& below = ng_.GridTile(index_ - csg::Point3(0, 1, 0));
   NavGridTile& above = ng_.GridTile(index_ + csg::Point3(0, 1, 0));
   above.UpdateBaseVectors();
   below.UpdateBaseVectors();
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
      NavGridTile* tile = !bottom_edge ? this : &below;
      csg::Point3 query_pt = !bottom_edge ? (pt - csg::Point3(0, 1, 0)) : csg::Point3(pt.x, TILE_SIZE-1, pt.z);

      NG_LOG(9) << "looking for support in " << tile->index_ << " pt " << query_pt;
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
               tile = &above;
            }
            NG_LOG(9) << "looking for air in " << tile->index_ << " pt " << riser 
                      << " (collision: " << tile->IsMarked(COLLISION, riser)
                      << " ladder: " << tile->IsMarked(LADDER, riser) << ")";
            bool empty_enough = !tile->IsMarked(COLLISION, riser) || tile->IsMarked(LADDER, riser);
            if (!empty_enough) {
               break;
            }
            riser.y++;
         }
         if (i == CHARACTER_HEIGHT) {
            NG_LOG(7) << "adding " << (bounds_.min + pt) << " to navgrid (offset" << pt << ")";
            can_stand_[Offset(pt)] = true;
            count++;
         }
      }
   }
   NG_LOG(5) << "marked " << count << " bits in can_stand vector.";
}
