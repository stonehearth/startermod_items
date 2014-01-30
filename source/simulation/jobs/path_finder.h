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
#include "lib/perfmon/namespace.h"
#include "path_finder_node.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class PathFinder;
class PathFinderSrc;
class PathFinderDst;
class Simulation;

DECLARE_SHARED_POINTER_TYPES(PathFinder)

class PathFinder : public std::enable_shared_from_this<PathFinder>,
                   public Job {
   public:
      static std::shared_ptr<PathFinder> Create(Simulation& sim, std::string name, om::EntityPtr entity);
      static void ComputeCounters(perfmon::Store& store);

   private:
      PathFinder(Simulation& sim, std::string name, om::EntityPtr source);

   public:
      virtual ~PathFinder();

      PathFinderPtr AddDestination(om::EntityRef dst);
      PathFinderPtr RemoveDestination(dm::ObjectId id);

      PathFinderPtr SetSource(csg::Point3 const& source);
      PathFinderPtr SetSolvedCb(luabind::object solved);
      PathFinderPtr SetSearchExhaustedCb(luabind::object solved);
      PathFinderPtr SetFilterFn(luabind::object dst_filter);
      PathFinderPtr RestartSearch(const char* reason);
      PathFinderPtr Start();
      PathFinderPtr Stop();

      PathPtr GetSolution() const;

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
      void SetSearchExhausted();
      csg::Point3 GetSourceLocation();

   private:
      static std::vector<std::weak_ptr<PathFinder>> all_pathfinders_;

      om::EntityRef                                entity_;
      luabind::object                              solved_cb_;
      luabind::object                              dst_filter_;
      luabind::object                              search_exhausted_cb_;
      bool                                         search_exhausted_;
      int                                          max_cost_to_destination_;
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

