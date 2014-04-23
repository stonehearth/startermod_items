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
   DirectPathFinder(Simulation &sim, om::EntityRef entityRef);

public:
   std::shared_ptr<DirectPathFinder> SetStartLocation(csg::Point3 const& startLocation);
   std::shared_ptr<DirectPathFinder> SetEndLocation(csg::Point3 const& endLocation);
   std::shared_ptr<DirectPathFinder> SetDestinationEntity(om::EntityRef destinationRef);
   std::shared_ptr<DirectPathFinder> SetAllowIncompletePath(bool allowIncompletePath);
   std::shared_ptr<DirectPathFinder> SetReversiblePath(bool reversiblePath);
   PathPtr GetPath();

private:
   bool GetEndPoints(csg::Point3& start, csg::Point3& endLocation) const;
   csg::Point3 GetPointOfInterest(csg::Point3 const& end) const;

   Simulation& sim_;
   csg::Point3 startLocation_;
   csg::Point3 endLocation_;
   om::EntityRef entityRef_;
   om::EntityRef destinationRef_;
   bool useEntityForStartPoint_;
   bool useEntityForEndPoint_;
   bool allowIncompletePath_;
   bool reversiblePath_;
   int logLevel_;
};

std::ostream& operator<<(std::ostream& os, DirectPathFinder const& o);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_DIRECT_PATH_FINDER_H
