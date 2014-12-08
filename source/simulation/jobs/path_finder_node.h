#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H

#include <vector>
#include "namespace.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

struct PathFinderNode
{
   PathFinderNode const* prev;
   csg::Point3 pt;
   float f, g;

   static inline bool CompareFitness(PathFinderNode *& lhs, PathFinderNode *& rhs) {
      return lhs->f > rhs->f;
   };
};


typedef std::vector<PathFinderNode> PathFinderNodeList;

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H

