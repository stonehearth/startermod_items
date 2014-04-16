#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H

#include <vector>
#include "namespace.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

struct PathFinderNode
{
   csg::Point3 pt;
   float f, g;

   PathFinderNode (csg::Point3 const& p) :
      pt(p), f(FLT_MAX), g(FLT_MAX)
   {
   }

   PathFinderNode (csg::Point3 const& p, float _f, float _g) :
      pt(p), f(_f), g(_g)
   {
   }

   static inline bool CompareFitness(PathFinderNode const& lhs, PathFinderNode const& rhs) {
      return lhs.f > rhs.f;
   };
};

typedef std::vector<PathFinderNode> PathFinderNodeList;

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H

