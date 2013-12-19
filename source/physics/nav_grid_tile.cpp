#include "radiant.h"
#include "radiant_stdutil.h"
#include "csg/region.h"
#include "csg/color.h"
#include "csg/util.h"
#include "nav_grid_tile.h"
#include "collision_tracker.h"
#include "protocols/radiant.pb.h"

using namespace radiant;
using namespace radiant::phys;

#define NG_LOG(level)              LOG(physics.navgrid, level)

NavGridTile::NavGridTile(NavGrid& ng, csg::Point3 const& index) :
   ng_(ng),
   dirty_(false)
{
   NG_LOG(5) << "!!!!!!!!!!!!!!!!!!";
   bounds_.min = index.Scaled(TILE_SIZE);
   bounds_.max = bounds_.min + csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void NavGridTile::RemoveCollisionTracker(TrackerType type, CollisionTrackerPtr tracker)
{
   dirty_ = true;
   stdutil::FastRemove(trackers_[type], tracker);
}

void NavGridTile::AddCollisionTracker(TrackerType type, CollisionTrackerPtr tracker)
{
   dirty_ = true;
   stdutil::UniqueInsert(trackers_[type], tracker);
}

// Cube is in navgrid coordinates
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

bool NavGridTile::CanStandOn(csg::Point3 const& pt)
{
   FlushDirty();
   return can_stand_[Offset(pt)];
}

bool NavGridTile::IsMarked(TrackerType type, csg::Point3 const& offest)
{
   return IsMarked(type, Offset(offest));
}

bool NavGridTile::IsMarked(TrackerType type, int bit_index)
{
   return marked_[type][bit_index];
}

int NavGridTile::Offset(csg::Point3 const& pt)
{
   ASSERT(pt.x >= 0 && pt.y >= 0 && pt.z >= 0);
   ASSERT(pt.x < TILE_SIZE && pt.y < TILE_SIZE && pt.z < TILE_SIZE);
   return TILE_SIZE * (pt.z + (TILE_SIZE * pt.y)) + pt.x;
}

void NavGridTile::FlushDirty()
{
   if (dirty_) {
      for (int i = 0; i < MAX_TRACKER_TYPES; i++) {
         TrackerType type = (TrackerType)i;
         marked_[type].reset();
         stdutil::ForEachPrune<CollisionTracker>(trackers_[type], [type, this](CollisionTrackerPtr tracker) {
            ProcessCollisionTracker(type, *tracker);
         });
      }
      UpdateCanStand();
      dirty_ = false;
   }
}

void NavGridTile::ProcessCollisionTracker(TrackerType type, CollisionTracker const& tracker)
{
   csg::Region3 overlap = tracker.GetOverlappingRegion(bounds_).Translated(-bounds_.min);
   for (csg::Cube3 const& cube : overlap) {
      for (csg::Point3 const& pt : cube) {
         NG_LOG(5) << "marking " << pt << " in bitvector " << type;
         marked_[type][Offset(pt)] = true;
      }
   }
}

void NavGridTile::ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg)
{
   csg::Color4 blocked_color(0, 0, 255, 128);
   csg::Color4 border_color(0, 0, 128, 128);

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

void NavGridTile::UpdateCanStand()
{
   int i;
   static int CHARACTER_HEIGHT = 4;
   csg::Cube3 bounds(csg::Point3::zero, csg::Point3(TILE_SIZE, TILE_SIZE, TILE_SIZE));

   can_stand_.reset();
   for (csg::Point3 pt : bounds) {
      // xxx:
      if (pt.y == 0 || pt.y >= TILE_SIZE - CHARACTER_HEIGHT) {
         continue;
      }

      int base = Offset(pt - csg::Point3(0, 1, 0));
      if (IsMarked(COLLISION, base) || IsMarked(LADDER, base)) {
         csg::Point3 riser = pt;
         for (i = 0; i < CHARACTER_HEIGHT; i++) {
            bool empty_enough = !IsMarked(COLLISION, riser) || IsMarked(LADDER, riser);
            if (!empty_enough) {
               break;
            }
            riser.y++;
         }
         if (i == CHARACTER_HEIGHT) {
            NG_LOG(5) << "adding " << (bounds_.min + pt) << " to navgrid (offset" << pt << ")";
            can_stand_[Offset(pt)] = true;
         }
      }
   }
}
