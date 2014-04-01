#include "pch.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define SIM_LOG(level)   LOG(simulation.core, level)

bool MovementHelpers::GetClosestPointAdjacentToEntity(Simulation& sim, csg::Point3 const& from, om::EntityPtr const& entity, csg::Point3& closestPoint)
{
   csg::Region3 const region = MovementHelpers::GetRegionAdjacentToEntity(sim, entity);

   if (region.IsEmpty()) {
      return false;
   }

   closestPoint = region.GetClosestPoint(from);
   return true;
}

csg::Region3 MovementHelpers::GetRegionAdjacentToEntity(Simulation& sim, om::EntityPtr const& entity)
{
   phys::OctTree const& octTree = sim.GetOctTree();
   csg::Region3 region;

   om::MobPtr const mob = entity->GetComponent<om::Mob>();
   if (!mob) {
      return region;
   }

   csg::Point3 const origin = mob->GetWorldGridLocation();

   om::Region3BoxedPtr adjacent;
   om::DestinationPtr const destination = entity->GetComponent<om::Destination>();
   if (destination) {
      adjacent = destination->GetAdjacent();
   }
   if (adjacent) {
      region = adjacent->Get();
      region.Translate(origin);
      octTree.RemoveNonStandableRegion(entity, region);
   } else {
      static csg::Point3 defaultAdjacentPoints[] = {
         csg::Point3(-1, 0,  0),
         csg::Point3( 1, 0,  0),
         csg::Point3( 0, 0, -1),
         csg::Point3( 0, 0,  1)
      };
      for (csg::Point3 const& point : defaultAdjacentPoints) {
         csg::Point3 location = origin + point;
         if (octTree.CanStandOn(entity, location)) {
            region.AddUnique(csg::Cube3(location));
         }
      }
   }

   return region;
}

csg::Point3 MovementHelpers::GetPointOfInterest(csg::Point3 const& adjacentPoint, om::EntityPtr const& entity)
{
   // Translate the point to the local coordinate system
   csg::Point3 origin(0, 0, 0);

   om::MobPtr mob = entity->GetComponent<om::Mob>();
   if (mob) {
      origin = mob->GetWorldGridLocation();
   }

   csg::Point3 poi(0, 0, 0);
   om::DestinationPtr dst = entity->GetComponent<om::Destination>();

   if (dst) {
      DEBUG_ONLY(
         csg::Region3 rgn = **dst->GetRegion();
         if (dst->GetReserved()) {
            rgn -= **dst->GetReserved();
         }

         if (dst->GetAutoUpdateAdjacent()) {
            csg::Region3 const& adjacent = **dst->GetAdjacent();

            //ASSERT(world_space_adjacent_region_.GetArea() <= adjacent.GetArea());
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
      poi = dst->GetPointOfInterest(adjacentPoint - origin);
   }

   poi += origin;

   if ((csg::Point2(adjacentPoint.x, adjacentPoint.z) - csg::Point2(poi.x, poi.z)).LengthSquared() != 1) {
      SIM_LOG(5) << "warning: distance from adjacentPoint " << adjacentPoint << " to " << poi << " is not 1.";
   }

   return poi;
}

template <class T>
bool MovementHelpers::CanStandOnOneOf(Simulation& sim, om::EntityPtr const& entity, std::vector<csg::Point<T,3>> points, csg::Point<T,3>& standablePoint)
{
   phys::OctTree const& octTree = sim.GetOctTree();

   for (csg::Point<T,3> const& point : points) {
      if (octTree.CanStandOn(entity, csg::ToClosestInt(point))) {
         standablePoint = point;
         return true;
      }
   }
   return false;
}

// returns true if this is a legal move (doesn't check adjacency yet though due to high game speed issues)
// resolvedLocation contains the updated standing location
// not returning nullable pointer because this function will get called a lot and we want to avoid the memory allocation
template <class T>
bool MovementHelpers::TestMoveXZ(Simulation& sim, om::EntityPtr const& entity,
                                 csg::Point<T,3> const& fromLocation, csg::Point<T,3> const& toLocation, csg::Point<T,3>& resolvedLocation)
{
   static csg::Point<T,3> delta_arr[] = {
      csg::Point<T,3>::zero,   // test the flat case first
      csg::Point<T,3>::unitY,  // then up before down
     -csg::Point<T,3>::unitY
                               // not testing down 2 yet since its not reversible and used by vector mover
   };
   static std::vector<csg::Point<T,3>> deltas(std::begin(delta_arr), std::end(delta_arr));

   //ASSERT((toLocation - fromLocation).LengthSquared() <= 2); // Can't do this yet

   phys::OctTree const& octTree = sim.GetOctTree();
   std::vector<csg::Point<T,3>> candidates;
   csg::Point<T,3> result;
   csg::Point<T,3> toLocationProjected = toLocation;
   toLocationProjected.y = fromLocation.y;
   bool passable;

   for (auto const& point : deltas) {
      candidates.push_back(toLocationProjected + point);
   }

   // test height requirements
   passable = MovementHelpers::CanStandOnOneOf(sim, entity, candidates, result);
   if (!passable) {
      return false;
   }

   // enforce rules for diagonal moves
   if ((fromLocation.x != result.x) && (fromLocation.z != result.z)) {
      csg::Point<T,3> side1 = result;
      side1.x = fromLocation.x;
      if (!octTree.CanStandOn(entity, csg::ToClosestInt(side1))) {
         return false;
      }

      csg::Point<T,3> side2 = result;
      side2.z = fromLocation.z;
      if (!octTree.CanStandOn(entity, csg::ToClosestInt(side2))) {
         return false;
      }
   }

   resolvedLocation = result;
   return true;
}

template bool MovementHelpers::TestMoveXZ(Simulation&, om::EntityPtr const&, csg::Point3 const&, csg::Point3 const&, csg::Point3&);
template bool MovementHelpers::TestMoveXZ(Simulation&, om::EntityPtr const&, csg::Point3f const&, csg::Point3f const&, csg::Point3f&);

// uses the version of Bresenham's line algorithm from Wikipedia
std::vector<csg::Point3> MovementHelpers::GetPathPoints(Simulation& sim, om::EntityPtr const& entity, csg::Point3 const& start, csg::Point3 const& end)
{
   int const x0 = start.x;
   int const z0 = start.z;
   int const x1 = end.x;
   int const z1 = end.z;
   int const dx = std::abs(x1 - x0);
   int const dz = std::abs(z1 - z0);
   int const sx = x0 < x1 ? 1 : -1;
   int const sz = z0 < z1 ? 1 : -1;
   int error = dx - dz;
   int error2;
   csg::Point3 current(start);
   csg::Point3 proposed;
   csg::Point3 next;
   std::vector<csg::Point3> result;
   bool passable;

   // no need to add the starting point to the result
   while (true) {
      error2 = error*2;
      proposed = current;

      if (error2 > -dz) {
         error -= dz;
         proposed.x = current.x + sx;
      }

      if (error2 < dx) {
         error += dx;
         proposed.z = current.z + sz;
      }

      passable = MovementHelpers::TestMoveXZ(sim, entity, current, proposed, next);
      if (!passable) {
         break;
      }

      result.push_back(next);

      if (next.x == x1 && next.z == z1) {
         break;
      }

      current = next;
   }

   return result;
}
