#ifndef _RADIANT_SIMULATION_JOBS_BFS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_BFS_PATH_FINDER_H

#include "om/om.h"
#include "job.h"
#include "protocols/radiant.pb.h"
#include "path.h"
#include "path_finder.h"
#include "csg/point.h"
#include "csg/color.h"

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
      static std::shared_ptr<BfsPathFinder> Create(Simulation& sim, std::string name, om::EntityPtr entity);
      static void ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter);
      virtual ~BfsPathFinder();

   private:
      BfsPathFinder(Simulation& sim, std::string name, om::EntityPtr source);

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
      bool IsSearchExhausted() const;

      PathPtr GetSolution() const;

      int EstimateCostToSolution();
      std::ostream& Format(std::ostream& o) const;
      std::string DescribeProgress();

   public: // Job Interface
      bool IsIdle() const override;
      bool IsFinished() const override { return false; }
      void Work(const platform::timer &timer) override;
      std::string GetProgress() const override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;

   private:
      static std::vector<std::weak_ptr<BfsPathFinder>> all_pathfinders_;
      om::EntityRef        entity_;
      PathPtr              solution_;
      AStarPathFinderPtr   pathfinder_;
      SolvedCb             solved_cb_;
      ExhaustedCb          exhausted_cb_;
      FilterFn             filter_fn_;
};

std::ostream& operator<<(std::ostream& o, const BfsPathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_BFS_PATH_FINDER_H

