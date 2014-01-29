#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

#include "om/om.h"
#include "job.h"
#include "physics/namespace.h"
#include "protocols/radiant.pb.h"
#include "path.h"
#include "csg/point.h"
#include "csg/color.h"
#include "om/region.h"
#include "path_finder_node.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class PathFinder;
class PathFinderSrc;
class PathFinderDst;
class Simulation;

class PathFinder : public Job {
   public:
      PathFinder(Simulation& sim, std::string name, om::EntityPtr source);
      virtual ~PathFinder();

      void AddDestination(om::EntityRef dst);
      void RemoveDestination(dm::ObjectId id);

      void SetSource(csg::Point3 const& source);
      void SetSolvedCb(luabind::object solved);
      void SetFilterFn(luabind::object dst_filter);

      PathPtr GetSolution() const;

      void RestartSearch(const char* reason);
      void Start();
      void Stop();
      int EstimateCostToSolution();
      std::ostream& Format(std::ostream& o) const;
      void SetDebugColor(csg::Color4 const& color);
      std::string DescribeProgress();

   public: // Job Interface
      bool IsIdle() const override;
      bool IsFinished() const override { return false; }
      void Work(const platform::timer &timer) override;
      std::string GetProgress() const override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;

   private:
      friend PathFinderSrc;
      friend PathFinderDst;
      void Restart();
      bool IsSearchExhausted() const;

   private:
      void RecommendBestPath(std::vector<csg::Point3> &points) const;
      int EstimateCostToDestination(const csg::Point3 &pt) const;
      int EstimateCostToDestination(const csg::Point3 &pt, PathFinderDst** closest) const;

      PathFinderNode GetFirstOpen();
      void ReconstructPath(std::vector<csg::Point3> &solution, const csg::Point3 &dst) const;
      void AddEdge(const PathFinderNode &current, const csg::Point3 &next, int cost);
      void RebuildHeap();

      void SolveSearch(const csg::Point3& last, PathFinderDst* dst);
      csg::Point3 GetSourceLocation();

   private:
      om::EntityRef                                entity_;
      luabind::object                              solved_cb_;
      luabind::object                              dst_filter_;
      int                                          costToDestination_;
      bool                                         rebuildHeap_;
      bool                                         restart_search_;
      bool                                         enabled_;
      mutable PathPtr                              solution_;
      csg::Color4                                  debug_color_;
   
      std::vector<PathFinderNode>                  open_;
      std::set<csg::Point3>                        closed_;
      std::unordered_map<csg::Point3, csg::Point3, csg::Point3::Hash>  cameFrom_;
   
      std::unique_ptr<PathFinderSrc>               source_;
      mutable std::unordered_map<dm::ObjectId, std::unique_ptr<PathFinderDst>>  destinations_;
};

std::ostream& operator<<(std::ostream& o, const PathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

