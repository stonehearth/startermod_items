#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H

#include <vector>
#include "namespace.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

struct PathFinderSolutionNode
{
   csg::Point3 pt;
   PathFinderSolutionNode *prev;

   PathFinderSolutionNode(csg::Point3 const&p, PathFinderSolutionNode* pr) :
      pt(p), prev(pr)
   {
   }

   ~PathFinderSolutionNode()
   {
      delete prev;
      prev = nullptr;
   }
};

struct PathFinderNode
{
   PathFinderSolutionNode* solutionNode;
   csg::Point3 pt;
   float f, g;

   PathFinderNode (csg::Point3 const& p) :
      pt(p), f(FLT_MAX), g(FLT_MAX), solutionNode(nullptr)
   {
   }

   PathFinderNode (csg::Point3 const& p, float _f, float _g, PathFinderSolutionNode* solNode) :
      pt(p), f(_f), g(_g), solutionNode(solNode)
   {
   }

   static inline bool CompareFitness(PathFinderNode const& lhs, PathFinderNode const& rhs) {
      return lhs.f > rhs.f;
   };
};


typedef std::vector<PathFinderNode> PathFinderNodeList;

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_NODE_H

