#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

#include "job.h"
#include "namespace.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class PathFinder : public Job {
   public:
      PathFinder(Simulation& sim, std::string name) : Job(sim, name) { }
      virtual ~PathFinder() { };

      virtual int EstimateCostToSolution() = 0;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
