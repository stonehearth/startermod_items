#ifndef _RADIANT_SIMULATION_MOVEMENT_HELPERS_H
#define _RADIANT_SIMULATION_MOVEMENT_HELPERS_H

#include "task.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class MovementHelper
{
public:
   MovementHelper(int _logLevel = 0);

   bool GetClosestPointAdjacentToEntity(Simulation& sim, csg::Point3f const& from, om::EntityPtr const& srcEntity, om::EntityPtr const& dstEntity, csg::Point3f& closestPoint) const;
   csg::Region3f GetRegionAdjacentToEntity(Simulation& sim, om::EntityPtr const& srcEntity, om::EntityPtr const& dstEntity) const;
   bool GetPointOfInterest(om::EntityPtr const& entity, csg::Point3f const& from, csg::Point3f& poi) const;
   bool GetPathPoints(Simulation& sim, om::EntityPtr const& entity, bool reversible, csg::Point3f const& start, csg::Point3f const& end, std::vector<csg::Point3f> &result) const;
   std::vector<csg::Point3f> PruneCollinearPathPoints(std::vector<csg::Point3f> const& points) const;
   bool TestAdjacentMove(Simulation& sim, om::EntityPtr entity, bool const reversible,
                         csg::Point3 const& fromLocation, int dx, int dz, csg::Point3& resolvedLocation) const;

private:
   enum Axis {
      UNDEFINED = 0,
      X = 1,
      Y = 2,
      Z = 3
   };

   std::vector<csg::Point3f> PruneCollinearPathPointsPlanar(std::vector<csg::Point3f> const& points) const;
   Axis MovementHelper::GetMajorAxis(csg::Point3 const& delta) const;
   bool CoordinateAdvancedAlongAxis(csg::Point3f const& segmentStart, csg::Point3f const& previous, csg::Point3f const& current, Axis axis) const;
   void GetSlopeBounds(csg::Point3 const& delta, Axis axis, float& maxSlope, float& minSlope, float& centerSlope) const;
   void TransposeDiagonalSlope(csg::Point3 const& delta, float& maxSlope, float& minSlope) const;

private:
   int      _logLevel;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_MOVEMENT_HELPERS_H
