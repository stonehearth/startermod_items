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

/*
 * -- BfsPathFinder
 *
 * The Breadth First Search pathfinder searches for the first destination that
 * matches the specified filter (see BfsPathFinder::SetFilterFn)
 *
 */

class BfsPathFinder : public std::enable_shared_from_this<BfsPathFinder>,
                      public PathFinder {
   public:
      static std::shared_ptr<BfsPathFinder> Create(om::EntityPtr entity, std::string const& name, int range);
      static void ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter);
      virtual ~BfsPathFinder();

   private:
      BfsPathFinder(om::EntityPtr entity, std::string const& name, int range);

   public:
      typedef std::function<bool(PathPtr)> SolvedCb;
      typedef std::function<void()> ExhaustedCb;

      om::EntityRef GetEntity() const;
      BfsPathFinderPtr SetSource(csg::Point3f const& source);
      BfsPathFinderPtr SetSolvedCb(SolvedCb const& solved_cb);
      BfsPathFinderPtr SetSearchExhaustedCb(ExhaustedCb const& exhausted_cb);
      BfsPathFinderPtr SetFilterResultCache(FilterResultCachePtr filterResultCache);
      BfsPathFinderPtr Start();
      BfsPathFinderPtr Stop();
      BfsPathFinderPtr ReconsiderDestination(om::EntityRef e);

      PathPtr GetSolution() const;
      float EstimateCostToSolution();
      void Log(uint level, std::string const& s);

   public: // Job Interface
      bool IsIdle() const override;
      bool IsFinished() const override { return false; }
      void Work(platform::timer const& timer) override;
      std::string GetProgress() override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;
      bool OpenSetContains(csg::Point3 const& pt) override;
      void GetPathFinderInfo(json::Node& info) override;

   private:
      void ExploreNextNode(csg::Point3 const& src);
      void ExpandSearch(platform::timer const& timer);
      void AddTileToSearch(csg::Point3 const& index);
      void ConsiderEntity(om::EntityPtr entity);
      void ConsiderAddedEntity(om::EntityPtr entity);
      float GetMaxExploredDistance() const;

   private:
      static std::vector<std::weak_ptr<BfsPathFinder>> all_pathfinders_;
      om::EntityRef        entity_;
      AStarPathFinderPtr   pathfinder_;
      ExhaustedCb          exhausted_cb_;
      FilterResultCachePtr filterResultCache_;
      int                  destinationCount_;
      int                  search_order_index_;
      float                explored_distance_;
      float                travel_distance_;
      float                max_travel_distance_;
      bool                 running_;
      dm::TracePtr         entityAddedTrace_;
};

std::ostream& operator<<(std::ostream& o, const BfsPathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_BFS_PATH_FINDER_H

