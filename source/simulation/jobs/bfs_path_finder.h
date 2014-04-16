#ifndef _RADIANT_SIMULATION_JOBS_BFS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_BFS_PATH_FINDER_H

#include "om/om.h"
#include "job.h"
#include "protocols/radiant.pb.h"
#include "path.h"
#include "path_finder.h"
#include "csg/point.h"
#include "csg/color.h"
#include <unordered_set>

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class BfsPathFinder;
class BfsPathFinderSrc;
class BfsPathFinderDst;
class Simulation;

DECLARE_SHARED_POINTER_TYPES(BfsPathFinder)

class BfsPathFinder : public std::enable_shared_from_this<BfsPathFinder>,
                      public PathFinder {
   public:
      static std::shared_ptr<BfsPathFinder> Create(Simulation& sim, om::EntityPtr entity, std::string name, int range);
      static void ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter);
      virtual ~BfsPathFinder();

   private:
      BfsPathFinder(Simulation& sim, om::EntityPtr entity, std::string const& name, int range);

   public:
      typedef std::function<void(PathPtr)> SolvedCb;
      typedef std::function<void()> ExhaustedCb;
      typedef std::function<bool(om::EntityPtr)> FilterFn;

      BfsPathFinderPtr SetSource(csg::Point3 const& source);
      BfsPathFinderPtr SetSolvedCb(SolvedCb solved_cb);
      BfsPathFinderPtr SetSearchExhaustedCb(ExhaustedCb exhausted_cb);
      BfsPathFinderPtr SetFilterFn(FilterFn filter_fn);
      BfsPathFinderPtr RestartSearch(const char* reason);
      BfsPathFinderPtr Start();
      BfsPathFinderPtr Stop();
      BfsPathFinderPtr ReconsiderDestination(om::EntityRef e);
      bool IsSearchExhausted() const;

      PathPtr GetSolution() const;

      float EstimateCostToSolution();
      std::ostream& Format(std::ostream& o) const;
      std::string DescribeProgress();

   public: // Job Interface
      bool IsIdle() const override;
      bool IsFinished() const override { return false; }
      void Work(platform::timer const& timer) override;
      std::string GetProgress() const override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;

   private:
      void ExploreNextNode(csg::Point3 const& src);
      void ExpandSearch(platform::timer const& timer);
      void AddTileToSearch(csg::Point3 const& index);
      void ConsiderEntity(om::EntityPtr entity);
      float GetMaxExploredDistance() const;

   private:
      static std::vector<std::weak_ptr<BfsPathFinder>> all_pathfinders_;
      om::EntityRef        entity_;
      PathPtr              solution_;
      AStarPathFinderPtr   pathfinder_;
      ExhaustedCb          exhausted_cb_;
      FilterFn             filter_fn_;
      int                  search_order_index_;
      float                explored_distance_;
      float                travel_distance_;
      float                max_travel_distance_;
      bool                 running_;
      std::unordered_set<dm::ObjectId> visited_ids_;
};

std::ostream& operator<<(std::ostream& o, const BfsPathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_BFS_PATH_FINDER_H

