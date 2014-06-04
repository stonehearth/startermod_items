#include "pch.h"
#include "movement_helpers.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "om/components/destination.ridl.h"
#include "simulation/simulation.h"
#include "physics/physics_util.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define MH_LOG(level)   LOG_I(_logLevel, level, "movement helpers")

MovementHelper::MovementHelper(int logLevel) :
   _logLevel(logLevel)
{
}

bool MovementHelper::GetClosestPointAdjacentToEntity(Simulation& sim, csg::Point3 const& from, om::EntityPtr const& srcEntity, om::EntityPtr const& dstEntity, csg::Point3& closestPoint)
{
   csg::Region3 const region = MovementHelper::GetRegionAdjacentToEntity(sim, srcEntity, dstEntity);

   if (region.IsEmpty()) {
      MH_LOG(5) << "region is empty in GetClosestPointAdjacentToEntity.  returning false";
      return false;
   }

   closestPoint = region.GetClosestPoint(from);
   MH_LOG(5) << "returning pt " << closestPoint << " from GetClosestPointAdjacentToEntity.";
   return true;
}

csg::Region3 MovementHelper::GetRegionAdjacentToEntity(Simulation& sim, om::EntityPtr const& srcEntity, om::EntityPtr const& dstEntity)
{
   phys::OctTree& octTree = sim.GetOctTree();
   csg::Region3 region;

   if (!dstEntity || !srcEntity) {
      MH_LOG(5) << "invalid entity ptr.  returning empty region";
      return region;
   }

   om::MobPtr const mob = dstEntity->GetComponent<om::Mob>();
   if (!mob) {
      MH_LOG(5) << *dstEntity <<  " has no mob component.  returning empty region";
      return region;
   }

   csg::Point3 const origin = mob->GetWorldGridLocation();

   om::Region3BoxedPtr adjacent;
   om::DestinationPtr const destination = dstEntity->GetComponent<om::Destination>();
   if (destination) {
      adjacent = destination->GetAdjacent();
   }
   if (adjacent) {      
      region = phys::LocalToWorld(adjacent->Get(), dstEntity);
      octTree.GetNavGrid().RemoveNonStandableRegion(srcEntity, region);
   } else {
      MH_LOG(7) << *dstEntity << " has no destination.  iterating through points adjacent to item";
      static csg::Point3 defaultAdjacentPoints[] = {
         csg::Point3(-1, 0,  0),
         csg::Point3( 1, 0,  0),
         csg::Point3( 0, 0, -1),
         csg::Point3( 0, 0,  1)
      };
      for (csg::Point3 const& point : defaultAdjacentPoints) {
         csg::Point3 location = origin + point;
         if (octTree.GetNavGrid().IsStandable(srcEntity, location)) {
            MH_LOG(9) << location << " is standable by " << *srcEntity << ".  adding point.";
            region.AddUnique(csg::Cube3(location));
         } else {
            MH_LOG(9) << location << " is not standable by " << *srcEntity << ".  skipping point.";
         }
      }
   }

   MH_LOG(7) << "returning adjacent region of size " << region.GetArea() << " for " << *dstEntity;
   return region;
}

csg::Point3 MovementHelper::GetPointOfInterest(csg::Point3 const& adjacentPoint, om::EntityPtr const& entity)
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
      MH_LOG(5) << "warning: distance from adjacentPoint " << adjacentPoint << " to " << poi << " is not 1.";
   }

   return poi;
}

template <class T>
static std::vector<csg::Point<T,3>> GetElevations(csg::Point<T,3> const& location, bool const reversible)
{
   static csg::Point<T,3> elevations[] = {
      csg::Point<T,3>(0,  0, 0),   // test the flat case first
      csg::Point<T,3>(0,  1, 0),   // then up before down
      csg::Point<T,3>(0, -1, 0),
      csg::Point<T,3>(0, -2, 0)
   };

   static csg::Point<T,3> reversibleElevations[] = {
      elevations[0], elevations[1], elevations[2]
   };

   std::vector<csg::Point<T,3>> points;

   if (reversible) {
      for (auto const& offset : reversibleElevations) {
         points.push_back(location + offset);
      }
   } else {
      for (auto const& offset : elevations) {
         points.push_back(location + offset);
      }
   }

   return points;
}

// returns true if this is a legal move (doesn't check adjacency yet though due to high game speed issues)
// resolvedLocation contains the updated standing location
// reversible indicates whether to include moves that cannot be reversed and (may cause the entity to get stuck)
template <class T>
bool MovementHelper::TestAdjacentMove(Simulation& sim, om::EntityPtr const& entity, bool const reversible,
                                       csg::Point<T,3> const& fromLocation, csg::Point<T,3> const& toLocation, csg::Point<T,3>& resolvedLocation)
{
   phys::OctTree const& octTree = sim.GetOctTree();
   csg::Point<T,3> toLocationProjected = toLocation;
   toLocationProjected.y = fromLocation.y;

   std::vector<csg::Point<T,3>> const elevations = GetElevations(toLocationProjected, reversible);

   // find a valid elevation to move to
   csg::Point<T,3> finalToLocation;
   if (!octTree.CanStandOnOneOf(entity, elevations, finalToLocation)) {
      return false;
   }

   // enforce rules for adjacent moves
   if (!octTree.ValidMove(entity, reversible, csg::ToClosestInt(fromLocation), csg::ToClosestInt(finalToLocation))) {
      return false;
   }

   resolvedLocation = finalToLocation;
   return true;
}

template bool MovementHelper::TestAdjacentMove(Simulation&, om::EntityPtr const&, bool const, csg::Point3 const&, csg::Point3 const&, csg::Point3&);
template bool MovementHelper::TestAdjacentMove(Simulation&, om::EntityPtr const&, bool const, csg::Point3f const&, csg::Point3f const&, csg::Point3f&);

// returns the points on the direct line path from start to end
// if end is not reachable, returns as far as it could go
// uses the version of Bresenham's line algorithm from Wikipedia
std::vector<csg::Point3> MovementHelper::GetPathPoints(Simulation& sim, om::EntityPtr const& entity, bool reversible, csg::Point3 const& start, csg::Point3 const& end)
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

      passable = MovementHelper::TestAdjacentMove(sim, entity, reversible, current, proposed, next);
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
