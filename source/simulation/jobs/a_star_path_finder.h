#ifndef _RADIANT_SIMULATION_JOBS_A_STAR_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_A_STAR_PATH_FINDER_H

#include "om/om.h"
#include "path_finder.h"
#include "physics/namespace.h"
#include "protocols/radiant.pb.h"
#include "path.h"
#include "csg/point.h"
#include "csg/color.h"
#include "om/region.h"
#include "path_finder_node.h"
#include <set>

BEGIN_RADIANT_SIMULATION_NAMESPACE

class AStarPathFinder : public std::enable_shared_from_this<AStarPathFinder>,
                        public PathFinder
{
   public:
      static std::shared_ptr<AStarPathFinder> Create(Simulation& sim, std::string const& name, om::EntityPtr entity);
      static void ComputeCounters(std::function<void(const char*, double, const char*)> const& addCounter);
      virtual ~AStarPathFinder();

   private:
      AStarPathFinder(Simulation& sim, std::string const& name, om::EntityPtr source);

   public:
      typedef std::function<void(PathPtr)> SolvedCb;
      typedef std::function<void()> ExhaustedCb;
      typedef std::function<bool(om::EntityPtr)> FilterFn;

      AStarPathFinderPtr AddDestination(om::EntityRef dst);
      AStarPathFinderPtr RemoveDestination(dm::ObjectId id);

      AStarPathFinderPtr SetSource(csg::Point3 const& source);
      AStarPathFinderPtr SetSolvedCb(SolvedCb solved);
      AStarPathFinderPtr SetSearchExhaustedCb(ExhaustedCb exhausted);
      AStarPathFinderPtr RestartSearch(const char* reason);
      AStarPathFinderPtr Start();
      AStarPathFinderPtr Stop();

      bool IsSolved() const;
      bool IsSearchExhausted() const;

      PathPtr GetSolution() const;
      csg::Point3 GetSourceLocation() const;
      float GetTravelDistance();

      float EstimateCostToSolution();
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
      void WatchWorldPoint(csg::Point3 const& point);
      void WatchWorldRegion(csg::Region3 const& region);

   private:
      void WatchTile(csg::Point3 const& index);
      void RecommendBestPath(std::vector<csg::Point3> &points) const;
      float EstimateCostToDestination(const csg::Point3 &pt) const;
      float EstimateCostToDestination(const csg::Point3 &pt, PathFinderDst** closest) const;

      PathFinderNode PopClosestOpenNode();
      void ReconstructPath(std::vector<csg::Point3> &solution, const csg::Point3 &dst) const;
      void AddEdge(const PathFinderNode &current, const csg::Point3 &next, float cost);
      void RebuildHeap();

      void SolveSearch(std::vector<csg::Point3>& solution, PathFinderDst& dst);
      void SetSearchExhausted();
      void OnTileDirty(csg::Point3 const& index);
      void EnableWorldWatcher(bool enabled);
      bool FindDirectPathToDestination(csg::Point3 const& from, PathFinderDst &dst);
      void OnPathFinderDstChanged(PathFinderDst const& dst, const char* reason);
      void RebuildOpenHeuristics();

   private:
      static std::vector<std::weak_ptr<AStarPathFinder>> all_pathfinders_;

      om::EntityRef                 entity_;
      SolvedCb                      solved_cb_;
      ExhaustedCb                   exhausted_cb_;
      bool                          search_exhausted_;
      float                         max_cost_to_destination_;
      int                           costToDestination_;
      bool                          rebuildHeap_;
      bool                          restart_search_;
      bool                          enabled_;
      bool                          world_changed_;
      bool                          _rebuildOpenHeuristics;
      int                           direct_path_search_cooldown_;
      mutable PathPtr               solution_;
      csg::Color4                   debug_color_;
   
      core::Guard                   navgrid_guard_;
      std::vector<PathFinderNode>   open_;
      std::set<csg::Point3>         closed_;
      std::set<csg::Point3>         watching_tiles_;
      std::unordered_map<csg::Point3, csg::Point3, csg::Point3::Hash>  cameFrom_;
      std::vector<csg::Point3>      _directPathCandiate;
   
      std::unique_ptr<PathFinderSrc>               source_;
      mutable std::unordered_map<dm::ObjectId, std::unique_ptr<PathFinderDst>>  destinations_;
};

std::ostream& operator<<(std::ostream& o, const AStarPathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_A_STAR_PATH_FINDER_H

