#include "pch.h"
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

#define PF_LOG(level)   LOG_CATEGORY(simulation.pathfinder, level, pf_.GetName() << " dst " << id_)

PathFinderDst::PathFinderDst(PathFinder &pf, om::EntityRef e) :
   pf_(pf),
   entity_(e),
   moving_(false)
{
   ASSERT(!e.expired());
   id_ = entity_.lock()->GetObjectId();
   CreateTraces();
}

PathFinderDst::~PathFinderDst()
{
   DestroyTraces();
}

void PathFinderDst::CreateTraces()
{
   auto entity = entity_.lock();
   if (entity) {
      PF_LOG(7) << "creating traces";

      auto destination_may_have_changed = [this](const char* reason) {
         auto ep = entity_.lock();
         if (ep) {
            ClipAdjacentToTerrain();
            pf_.RestartSearch(reason);
         }
      };

      auto& o = GetSim().GetOctTree();
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         moving_ = mob->GetMoving();

         transform_trace_ = mob->TraceTransform("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([=](csg::Transform const&) {
                                    PF_LOG(7) << "mob transform changed";
                                    destination_may_have_changed("mob transform changed");
                                 });

         moving_trace_ = mob->TraceMoving("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([=](bool const& moving) {
                                    if (moving != moving_) {
                                       PF_LOG(7) << "mob moving changed";
                                       moving_ = moving;
                                       if (moving_) {
                                          pf_.RestartSearch("mob moving changed");
                                       }
                                    }
                                 });
      }
      auto dst = entity->GetComponent<om::Destination>();
      if (dst) {
         region_guard_ = dst->TraceAdjacent("pf dst", dm::PATHFINDER_TRACES)
                                 ->OnChanged([this, destination_may_have_changed](csg::Region3 const&) {
                                    PF_LOG(7) << "adjacent region changed";
                                    destination_may_have_changed("adjacent region changed");
                                 });
      }
      ClipAdjacentToTerrain();
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
            o.RemoveNonStandableRegion(entity, world_space_adjacent_region_);
         } else {
            static csg::Point3 delta[] = {
               csg::Point3(-1, 0,  0),
               csg::Point3( 1, 0,  0),
               csg::Point3( 0, 0, -1),
               csg::Point3( 0, 0,  1)
            };
            for (const auto& d : delta) {
               csg::Point3 location = origin + d;
               if (o.CanStandOn(entity, location)) {
                  world_space_adjacent_region_.AddUnique(csg::Cube3(location));
               }
            }
         }
      }
   }
}

int PathFinderDst::EstimateMovementCost(const csg::Point3& from) const
{
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
      csg::Region3 const& adj = *adjacent;
      if (adj.IsEmpty()) {
         return INT_MAX;
      }
      end = adj.GetClosestPoint(start);      
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
      DEBUG_ONLY(
         csg::Region3 rgn = **dst->GetRegion();
         if (dst->GetReserved()) {
            rgn -= **dst->GetReserved();
         }

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
      )
      end = dst->GetPointOfInterest(adjacent_pt - origin);
   }

   end += origin;
   // So, this ASSERT really means "equals 1".  Do not remove this assert.  If you see this assert
   // fire, something horribly wrong has happened.  This must always be true because animations
   // are tuned to drop things on adjacent spaces.  If we were to relax this constraint (e.g. is
   // it ok adjacent_pt == end?), the anmiation would play and the block would teleport to some
   // other location.  Not good!  So, unless we're happy with wonky animations, make sure the
   // distance is exactly 1.
   if ((adjacent_pt - end).LengthSquared() != 1) {
      PF_LOG(1) << "warning: distanced from adjacent_pt " << adjacent_pt << " to " << end << " is not 1.";
   }
   return end;
}

void PathFinderDst::DestroyTraces()
{
   PF_LOG(7) << "destroying traces";
   moving_trace_ = nullptr;
   transform_trace_ = nullptr;
   region_guard_ = nullptr;
}

bool PathFinderDst::IsIdle() const
{
   return world_space_adjacent_region_.IsEmpty();
}

void PathFinderDst::EncodeDebugShapes(radiant::protocol::shapelist *msg, csg::Color4 const& debug_color) const
{
   if (LOG_IS_ENABLED(simulation.pathfinder, 7)) {
      auto region = msg->add_region();
      debug_color.SaveValue(region->mutable_color());
      for (auto const& cube : world_space_adjacent_region_) {
         cube.SaveValue(region->add_cubes());
      }
   }
}

dm::ObjectId PathFinderDst::GetEntityId() const
{
   return id_;
}
