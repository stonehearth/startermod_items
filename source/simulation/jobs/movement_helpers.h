#ifndef _RADIANT_SIMULATION_MOVEMENT_HELPERS_H
#define _RADIANT_SIMULATION_MOVEMENT_HELPERS_H

#include "task.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class MovementHelpers
{
public:
   static bool GetClosestPointAdjacentToEntity(Simulation& sim, csg::Point3 const& from, om::EntityPtr const& entity, csg::Point3& closestPoint);
   static csg::Region3 GetRegionAdjacentToEntity(Simulation& sim, om::EntityPtr const& entity);
   static csg::Point3 GetPointOfInterest(csg::Point3 const& adjacentPoint, om::EntityPtr const& entity);
   static std::vector<csg::Point3> GetPathPoints(Simulation& sim, om::EntityPtr const& entity, bool reversible, csg::Point3 const& start, csg::Point3 const& end);

   template <class T>
   static bool TestAdjacentMove(Simulation& sim, om::EntityPtr const& entity, bool const reversible,
                          csg::Point<T,3> const& fromLocation, csg::Point<T,3> const& toLocation, csg::Point<T,3>& resolvedLocation);
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_MOVEMENT_HELPERS_H
