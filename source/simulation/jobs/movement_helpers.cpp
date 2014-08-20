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

bool MovementHelper::GetClosestPointAdjacentToEntity(Simulation& sim, csg::Point3 const& from, om::EntityPtr const& srcEntity, om::EntityPtr const& dstEntity, csg::Point3& closestPoint) const
{
   csg::Region3 region = MovementHelper::GetRegionAdjacentToEntity(sim, srcEntity, dstEntity);

   phys::OctTree& octTree = sim.GetOctTree();
   octTree.GetNavGrid().RemoveNonStandableRegion(srcEntity, region);

   if (region.IsEmpty()) {
      MH_LOG(5) << "region is empty in GetClosestPointAdjacentToEntity.  returning false";
      return false;
   }

   closestPoint = region.GetClosestPoint(from);
   MH_LOG(5) << "returning pt " << closestPoint << " from GetClosestPointAdjacentToEntity.";
   return true;
}

csg::Region3 MovementHelper::GetRegionAdjacentToEntity(Simulation& sim, om::EntityPtr const& srcEntity, om::EntityPtr const& dstEntity) const
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

   if (!mob->GetBone().empty()) {
      MH_LOG(5) << *dstEntity <<  " is attached to \"" << mob->GetBone() << "\" bone.  returning empty region";
      return region;
   }

   csg::Point3 const origin = mob->GetWorldGridLocation();
   MH_LOG(9) << *dstEntity << " location: " << origin;

   om::Region3BoxedPtr adjacent;
   om::DestinationPtr const destination = dstEntity->GetComponent<om::Destination>();
   if (destination) {
      adjacent = destination->GetAdjacent();
   }
   if (adjacent) {      
      MH_LOG(9) << "adjacent local region bounds: " << adjacent->Get().GetBounds();
      region = phys::LocalToWorld(adjacent->Get(), dstEntity);
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
         region.AddUnique(csg::Cube3(location));
      }
   }

   MH_LOG(7) << "returning adjacent region of size " << region.GetArea() << " for " << *dstEntity;
   return region;
}

csg::Point3 MovementHelper::GetPointOfInterest(csg::Point3 const& adjacentPointWorld, om::EntityPtr const& entity) const
{
   csg::Point3 adjacentPointLocal = phys::WorldToLocal(adjacentPointWorld, entity);
   csg::Point3 poiLocal(0, 0, 0);
   csg::Point3 poiWorld;

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
      poiLocal = dst->GetPointOfInterest(adjacentPointLocal);
   }

   poiWorld = phys::LocalToWorld(poiLocal, entity);

   if ((csg::Point2(adjacentPointWorld.x, adjacentPointWorld.z) - 
        csg::Point2(poiWorld.x, poiWorld.z)).LengthSquared() != 1) {
      MH_LOG(5) << "warning: distance from adjacentPoint " << adjacentPointWorld << " to " << poiWorld << " is not 1.";
   }

   return poiWorld;
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
                                       csg::Point<T,3> const& fromLocation, csg::Point<T,3> const& toLocation, csg::Point<T,3>& resolvedLocation) const
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

template bool MovementHelper::TestAdjacentMove(Simulation&, om::EntityPtr const&, bool const, csg::Point3 const&, csg::Point3 const&, csg::Point3&) const;
template bool MovementHelper::TestAdjacentMove(Simulation&, om::EntityPtr const&, bool const, csg::Point3f const&, csg::Point3f const&, csg::Point3f&) const;

// returns the points on the direct line path from start to end
// if end is not reachable, returns as far as it could go
// uses the version of Bresenham's line algorithm from Wikipedia
std::vector<csg::Point3> MovementHelper::GetPathPoints(Simulation& sim, om::EntityPtr const& entity, bool reversible, csg::Point3 const& start, csg::Point3 const& end) const
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

   // following convention, we include the origin point in the path
   // follow path can decide to skip this point so that we don't run to the origin of the current block first
   result.push_back(start);

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

bool MovementHelper::CoordinateAdvancedAlongAxis(csg::Point3 const& segmentStart, csg::Point3 const& previous, csg::Point3 const& current, Axis axis) const
{
   int d;

   switch (axis) {
      case Axis::X:
         d = 0; break;
      case Axis::Y:
         d = 1; break;
      case Axis::Z:
         d = 2; break;
      default:
         assert(false);
         return false;
   }

   int currentOffset = std::abs(current[d] - segmentStart[d]);
   int previousOffset = std::abs(previous[d] - segmentStart[d]);
   return currentOffset > previousOffset;
}

// Calculate the bounds of the slope of all lines that could contain this point.
// Slope has a normalized definition of minor axis / abs(major axis).
// This limits (center) slopes +/- 45 degrees of the positive x axis
// We can now think of all lines as being shallow lines travelling to the right.
// maxSlope is the slope bounds closest to +inf.
// minSlope is the slope bounds closest to -inf.
// Comparing floats should be ok, since we shouldn't have any floating point drift.
MovementHelper::Axis MovementHelper::GetSlopeBounds(csg::Point3 const& delta, float& maxSlope, float& minSlope, float& centerSlope) const
{
   int absdx = std::abs(delta.x);
   int absdz = std::abs(delta.z);
   int major, minor;
   float absMajor;
   Axis majorAxis;

   // Get the major and minor coordinates.
   // If equal, default to x-major axis. we'll fix this later if we deviate from the diagonal.
   if (absdx >= absdz) {
      major = delta.x;
      minor = delta.z;
      absMajor = (float) absdx;
      majorAxis = Axis::X;
   } else {
      major = delta.z;
      minor = delta.x;
      absMajor = (float) absdz;
      majorAxis = Axis::Z;
   }

   if (absMajor == 0) {
      assert(absdx == 0 && absdz == 0);
      return majorAxis;
   }

   // This point must be within 0.5 units of the actual line, otherwise the point above or below would have been closer.
   maxSlope = (minor + 0.5f) / absMajor;
   minSlope = (minor - 0.5f) / absMajor;
   centerSlope = minor / absMajor;

   // Magnitude of center slope can never exceed 1 because we have normalized it to be minor axis / abs(major axis).
   assert(centerSlope <=  1.0f);
   assert(centerSlope >= -1.0f);
   assert(maxSlope <=  1.5f);
   assert(minSlope >= -1.5f);

   return majorAxis;
}

void MovementHelper::TransposeDiagonalSlope(csg::Point3 const& delta, float& maxSlope, float& minSlope) const
{
   if ((delta.x ^ delta.z) < 0) {
      // minor = -major, recalculate the transposed slope values
      //
      // Given that:
      //   oldMaxSlope = (minor + 0.5f) / absMajor;
      //   oldMinSlope = (minor - 0.5f) / absMajor;
      //
      // The new equations are:
      //   newMaxSlope = (-minor + 0.5f) / absMajor;
      //               = -(minor - 0.5f) / absMajor;
      //               = -oldMinSlope;
      //
      //   newMinSlope = (-minor - 0.5f) / absMajor;
      //               = -(minor + 0.5f) / absMajor;
      //               = -oldMaxSlope;
      float temp = maxSlope;
      maxSlope = -minSlope;
      minSlope = -temp;
   } else {
      // minor == major, The transposed slope values are the same as the original values.
   }
}

// Uses the Bresenham line invariant to prune "collinear" points within a line segment.
// The invariant conditions are defined as follows:
//    1) Each end point of the segment are exactly on the line.
//    2) At each major axis coordinate, the minor axis coordinate is within 0.5 units of the line.
//       (Otherwise another point would be closer and that point woulld have been chosed instead.)
//
// This algorithm inverts Bresenham by estimating the true line from the points provided.
// The first point anchors one point of the line segment, and each additional point puts bounds
// on the set of slopes that can satisfy all points (being within 0.5 units of the line).
// So, a point can be part of the line segment if its slope bounds have a non-zero intersection,
// with the aggregate slope bounds and we add its bounding constraints to the aggregate.
// However, it cannot end the segment unless the point falls inside the aggregate bounds.
// i.e. there are points that can be part of the line segment, but cannot terminate the segment.
// e.g. this would never be a line drawn: (0,0) (1,0) (2,0), (3,1)
// because the (2,0) point is farther from the line than the (2,1) point.
//
// It seems like someone should have inverted Bresenham's algorithm before, but I couldn't find any reference.
std::vector<csg::Point3> MovementHelper::PruneCollinearPathPointsPlanar(std::vector<csg::Point3> const& points) const
{
   std::vector<csg::Point3> result;
   int numPoints = points.size();
   if (numPoints <= 2) {
      result = points; // Two points define a line - nothing to prune.
      return result;
   }

   csg::Point3 segmentStart = points[0];
   int segmentEndIndex = 0;

   // The normalized slope must be between -1.5 and 1.5. (i.e. The normalized center slope is betweeen -1 and 1).
   // Initialize with out of range values, so they get assigned.
   float const INVALID_SLOPE = 100.0f;
   float segmentMaxSlope = INVALID_SLOPE;
   float segmentMinSlope = -INVALID_SLOPE; 
   Axis segmentMajorAxis = Axis::UNDEFINED;
   
   // Following convention, we include the origin point in the path.
   result.push_back(segmentStart);

   for (int i = 1; i < numPoints; i++) {
      csg::Point3 previousPoint = points[i-1];
      csg::Point3 currentPoint = points[i];
      csg::Point3 delta = currentPoint - segmentStart;

      float maxSlope, minSlope, centerSlope;
      Axis pointMajorAxis = GetSlopeBounds(delta, maxSlope, minSlope, centerSlope);

      // Don't prune when the path doubles back on itself.
      // Don't prune when there are two points at the same major axis coordinate.
      // It is possible for two points with the same major axis coordinate to be equidistant from the line.
      // Both are equally valid, but only one of the points will be selected by bresenham (or most other line algorithms).
      bool majorAxisCoordinateAdvanced = CoordinateAdvancedAlongAxis(segmentStart, previousPoint, currentPoint, pointMajorAxis);

      if (majorAxisCoordinateAdvanced) {
         if (pointMajorAxis != segmentMajorAxis) {
            if (segmentMajorAxis != Axis::UNDEFINED) {
               // We have a diagonal line that just became non-diagonal.
               TransposeDiagonalSlope(delta, segmentMaxSlope, segmentMinSlope);
            } else {
               // Major axis defined on the second point of the segment, just assign it below.
            }
            segmentMajorAxis = pointMajorAxis;
         }

         // Add the slope constraints of the current point to the existing constraints.
         if (maxSlope < segmentMaxSlope) {
            segmentMaxSlope = maxSlope;
         }
         if (minSlope > segmentMinSlope) {
            segmentMinSlope = minSlope;
         }

         // Check if this point can end the segment
         // It can be part of the segment if the points bounds have a non-zero overlap with the segment bounds,
         // but it can only be an end point of the segment if the center is within the segment bounds.
         if (centerSlope >= segmentMinSlope && centerSlope <= segmentMaxSlope) {
            segmentEndIndex = i;
         }
      }

      bool lastPoint = i == numPoints-1;
      bool validSlope = segmentMinSlope <= segmentMaxSlope;

      // Check if we should end the segment.
      if (!validSlope || !majorAxisCoordinateAdvanced || lastPoint) {
         // Rewind to the last valid segment end point.
         i = segmentEndIndex;
         currentPoint = points[i];
         result.push_back(currentPoint);

         // Start a new segment.
         segmentStart = currentPoint;
         segmentMaxSlope = INVALID_SLOPE;
         segmentMinSlope = -INVALID_SLOPE;
         segmentMajorAxis = Axis::UNDEFINED;
      }
   }

   return result;
}

template <typename T>
static void AppendVector(std::vector<T>& x, std::vector<T> const& y)
{
   x.reserve(x.size() + y.size());
   x.insert(x.end(), y.begin(), y.end());
}

// Chops the points into XZ planar subsets and calls PruneCollinearPathPointsPlanar on each subset and concatenates the results.
std::vector<csg::Point3> MovementHelper::PruneCollinearPathPoints(std::vector<csg::Point3> const& points) const
{
   std::vector<csg::Point3> subsetPoints;
   std::vector<csg::Point3> subsetResult;
   std::vector<csg::Point3> result;

   int numPoints = points.size();
   if (numPoints <= 2) {
      result = points; // Two points define a line - nothing to prune.
      return result;
   }

   subsetPoints.push_back(points[0]);

   for (int i = 1; i < numPoints; i++) {
      csg::Point3 previousPoint = points[i-1];
      csg::Point3 currentPoint = points[i];

      // Elevation change, close the subset and process.
      if (currentPoint.y != previousPoint.y) {
         subsetResult = PruneCollinearPathPointsPlanar(subsetPoints);
         AppendVector(result, subsetResult);
         subsetPoints.clear();
      }

      if (currentPoint != previousPoint) {
         subsetPoints.push_back(currentPoint);
      }
   }

   // Process the last open subset.
   subsetResult = PruneCollinearPathPointsPlanar(subsetPoints);
   AppendVector(result, subsetResult);

   return result;
}
