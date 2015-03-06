#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

#include "job.h"
#include "namespace.h"
#include "csg/point.h"
#include "lib/json/node.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class PathFinder : public Job {
   public:
      PathFinder(std::string name) : Job(name) { }
      virtual ~PathFinder() { };

      virtual float EstimateCostToSolution() = 0;
      virtual bool OpenSetContains(csg::Point3 const& pt) = 0;
      virtual void GetPathFinderInfo(json::Node& info) = 0;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
