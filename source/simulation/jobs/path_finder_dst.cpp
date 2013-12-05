#include "pch.h"
#include "metrics.h"
#include "path.h"
#include "path_finder.h"
#include "path_finder_dst.h"
#include "simulation/simulation.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "om/region.h"
#include "csg/color.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

PathFinderDst::PathFinderDst(PathFinder &pf, om::EntityRef e) :
   pf_(pf),
   entity_(e),
   moving_(false),
   collision_cb_id_(0)
{
   ASSERT(!e.expired());
   auto entity = e.lock();
   if (entity) {
      auto destination_may_have_changed = [this, e]() {
         auto ep = e.lock();
         if (ep) {
            ClipAdjacentToTerrain();
            pf_.RestartSearch();
         }
      };

      auto& o = GetSim().GetOctTree();
      collision_cb_id_ = o.AddCollisionRegionChangeCb(&world_space_adjacent_region_, [destination_may_have_changed] {
         destination_may_have_changed();
      });

      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         moving_ = mob->GetMoving();

         transform_trace_ = mob->TraceTransform("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([=](csg::Transform const&) {
                                    destination_may_have_changed();
                                 });

         moving_trace_ = mob->TraceMoving("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([=](bool const& moving) {
                                    if (moving != moving_) {
                                       moving_ = moving;
                                       if (moving_) {
                                          pf_.RestartSearch();
                                       }
                                    }
                                 });
      }
      auto dst = entity->GetComponent<om::Destination>();
      if (dst) {
         region_guard_ = dst->TraceAdjacent("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([destination_may_have_changed](csg::Region3 const&) {
                                    destination_may_have_changed();
                                 });
      }
      ClipAdjacentToTerrain();
   }
}

PathFinderDst::~PathFinderDst()
{
   if (collision_cb_id_) {
      auto& o = GetSim().GetOctTree();
      o.RemoveCollisionRegionChangeCb(collision_cb_id_);
      collision_cb_id_ = 0;
   }
}


void PathFinderDst::ClipAdjacentToTerrain()
{
   const auto& o = GetSim().GetOctTree();
   world_space_adjacent_region_.Clear();

   if (moving_) {
      //return;
   }

   auto entity = entity_.lock();
   if (entity) {
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         csg::Point3 origin;
         origin = mob->GetWorldGridLocation();

         om::Region3BoxedPtr adjacent;
         auto destination = entity->GetComponent<om::Destination>();
         if (destination) {
            adjacent = destination->GetAdjacent();
         }
         if (adjacent) {
            world_space_adjacent_region_ = adjacent->Get();
            world_space_adjacent_region_.Translate(origin);
            o.ClipRegion(world_space_adjacent_region_);
         } else {
            static csg::Point3 delta[] = {
               csg::Point3(-1, 0,  0),
               csg::Point3( 1, 0,  0),
               csg::Point3( 0, 0, -1),
               csg::Point3( 0, 0,  1)
            };
            for (const auto& d : delta) {
               csg::Point3 location = origin + d;
               if (o.CanStand(location)) {
                  world_space_adjacent_region_.AddUnique(csg::Cube3(location));
               }
            }
         }
      }
   }
}

int PathFinderDst::EstimateMovementCost(const csg::Point3& from) const
{
   PROFILE_BLOCK();

   auto entity = GetEntity();

   if (!entity) {
      return INT_MAX;
   }

   // Translate the point to the local coordinate system
   csg::Point3 start = from;
   auto mob = entity->GetComponent<om::Mob>();
   if (mob) {
      start -= mob->GetWorldGridLocation();
   }

   csg::Point3 end;

   om::Region3BoxedPtr adjacent;
   om::DestinationPtr dst = entity->GetComponent<om::Destination>();
   if (dst) {
      adjacent = dst->GetAdjacent();
   }
   if (adjacent) {
      csg::Region3 const& rgn = *adjacent;
      if (rgn.IsEmpty()) {
         return INT_MAX;
      }
      end = rgn.GetClosestPoint(start);
   } else {
      csg::Region3 adjacent;
      adjacent += csg::Point3(-1, 0,  0);
      adjacent += csg::Point3( 1, 0,  0);
      adjacent += csg::Point3( 0, 0, -1);
      adjacent += csg::Point3( 0, 0, -1);
      end = adjacent.GetClosestPoint(start);
   }
   return EstimateMovementCost(start, end);
}

int PathFinderDst::EstimateMovementCost(csg::Point3 const& start, csg::Point3 const& end) const
{
   PROFILE_BLOCK();

   static int COST_SCALE = 10;
   int cost = 0;

   // it's fairly expensive to climb.
   cost += COST_SCALE * std::max(end.y - start.y, 0) * 2;

   // falling is super cheap.
   cost += std::max(start.y - end.y, 0);

   // diagonals need to be more expensive than cardinal directions
   int xCost = abs(end.x - start.x);
   int zCost = abs(end.z - start.z);
   int diagCost = std::min(xCost, zCost);
   int horzCost = std::max(xCost, zCost) - diagCost;

   cost += (int)((horzCost + diagCost * 1.414) * COST_SCALE);

   return cost;
}

csg::Point3 PathFinderDst::GetPointfInterest(csg::Point3 const& adjacent_pt) const
{
   PROFILE_BLOCK();

   auto entity = GetEntity();
   ASSERT(entity);

   // Translate the point to the local coordinate system
   csg::Point3 origin(0, 0, 0);
   auto mob = entity->GetComponent<om::Mob>();
   if (mob) {
      origin = mob->GetWorldGridLocation();
   }

   csg::Point3 end(0, 0, 0);
   om::DestinationPtr dst = entity->GetComponent<om::Destination>();
   if (dst) {
      csg::Region3 const& rgn = **dst->GetRegion();

      if (dst->GetAutoUpdateAdjacent()) {
         csg::Region3 const& adjacent = **dst->GetAdjacent();

         ASSERT(world_space_adjacent_region_.GetArea() <= adjacent.GetArea());
         for (const auto& cube : adjacent) {
            for (const auto& pt : cube) {
               csg::Point3 closest = rgn.GetClosestPoint(pt);
               csg::Point3 d = closest - pt;
               // cubes adjacent to one rect in the region might actually be contained
               // in another rect in the region!  therefore, just ensure that the distance
               // from the adjacent to the non-adjacent is <= 1 (not exactly 1)...
               ASSERT(d.LengthSquared() <= 1);
            }
         }
      }
      end = rgn.GetClosestPoint(adjacent_pt - origin);
   }

   end += origin;
   return end;
}

bool PathFinderDst::IsIdle() const
{
   return world_space_adjacent_region_.IsEmpty();
}

void PathFinderDst::EncodeDebugShapes(radiant::protocol::shapelist *msg, csg::Color4 const& debug_color) const
{
   auto region = msg->add_region();
   debug_color.SaveValue(region->mutable_color());
   for (auto const& cube : world_space_adjacent_region_) {
      cube.SaveValue(region->add_cubes());
   }
}
