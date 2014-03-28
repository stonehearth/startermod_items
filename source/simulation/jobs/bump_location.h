#ifndef _RADIANT_SIMULATION_BUMP_LOCATION_H
#define _RADIANT_SIMULATION_BUMP_LOCATION_H

#include "task.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class BumpLocation : public Task
{
public:
   BumpLocation(Simulation& sim, om::EntityRef entity, csg::Point3f const& vector);

public:
   bool Work(platform::timer const& timer) override;

protected:
   om::EntityRef entity_;
   csg::Point3f const vector_;
};

std::ostream& operator<<(std::ostream& os, BumpLocation const& o);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_BUMP_LOCATION_H
