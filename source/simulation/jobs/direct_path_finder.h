#ifndef _RADIANT_SIMULATION_DIRECT_PATH_FINDER_H
#define _RADIANT_SIMULATION_DIRECT_PATH_FINDER_H

#include "task.h"
#include "path.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class DirectPathFinder : public std::enable_shared_from_this<DirectPathFinder>
{
public:
   DirectPathFinder(Simulation &sim, om::EntityRef entityRef, om::EntityRef targetRef);

public:
   std::shared_ptr<DirectPathFinder> SetStartLocation(csg::Point3 const& startLocation);
   std::shared_ptr<DirectPathFinder> SetAllowIncompletePath(bool allowIncompletePath);
   PathPtr GetPath();

protected:
   Simulation& sim_;
   om::EntityRef entityRef_;
   om::EntityRef targetRef_;
   bool allowIncompletePath_;
   csg::Point3 startLocation_;
};

std::ostream& operator<<(std::ostream& os, DirectPathFinder const& o);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_DIRECT_PATH_FINDER_H
