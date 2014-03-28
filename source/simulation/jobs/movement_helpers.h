#ifndef _RADIANT_SIMULATION_MOVEMENT_HELPERS_H
#define _RADIANT_SIMULATION_MOVEMENT_HELPERS_H

#include "task.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class MovementHelpers
{
public:
   static bool GetStandableLocation(Simulation& sim, std::shared_ptr<om::Entity> const& entity, csg::Point3f& desiredLocation, csg::Point3f& actualLocation);
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_MOVEMENT_HELPERS_H
