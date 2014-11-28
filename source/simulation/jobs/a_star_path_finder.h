#ifndef _RADIANT_SIMULATION_JOBS_A_STAR_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_A_STAR_PATH_FINDER_H

#include "om/om.h"
#include "path_finder.h"
#include "physics/namespace.h"
#include "physics/octtree.h"
#include "protocols/radiant.pb.h"
#include "path.h"
#include "csg/point.h"
#include "csg/color.h"
#include "om/region.h"
#include "path_finder_node.h"
#include <unordered_set>
#include <boost/pool/object_pool.hpp>

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
      typedef std::function<bool(PathPtr)> SolvedCb;
      typedef std::function<void()> ExhaustedCb;
      typedef std::function<bool(om::EntityPtr)> FilterFn;

      AStarPathFinderPtr AddDestination(om::EntityRef dst);
      AStarPathFinderPtr RemoveDestination(dm::ObjectId id);

      AStarPathFinderPtr SetSource(csg::Point3f const& source);
      AStarPathFinderPtr SetSolvedCb(SolvedCb const& solved);
      AStarPathFinderPtr SetSearchExhaustedCb(ExhaustedCb const& exhausted);
      AStarPathFinderPtr RestartSearch(const char* reason);
      AStarPathFinderPtr Start();
      AStarPathFinderPtr Stop();
      void Destroy();

      bool IsSolved() const;
      bool IsSearchExhausted() const;

      om::EntityRef GetEntity() const;
      PathPtr GetSolution() const;
      csg::Point3f GetSourceLocation() const;
      float GetTravelDistance();

      float EstimateCostToSolution();
      std::ostream& Format(std::ostream& o) const;
      void SetDebugColor(csg::Color4 const& color);

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
      void WatchWorldRegion(csg::Region3f const& region);

   private:
      void WatchTile(csg::Point3 const& index);
      void RecommendBestPath(std::vector<csg::Point3f> &points) const;
      float EstimateCostToDestination(const csg::Point3 &pt) const;
      float EstimateCostToDestination(const csg::Point3 &pt, PathFinderDst** closest, float maxMod) const;
      float GetMaxMovementModifier(csg::Point3 const& point) const;

      PathFinderNode* PopClosestOpenNode();
      void ReconstructPath(std::vector<csg::Point3f> &solution, const PathFinderNode* dst) const;
      void AddEdge(const csg::Point3 &next, float cost);
      void RebuildHeap();

      bool SolveSearch(std::vector<csg::Point3f>& solution, PathFinderDst*& dst);
      void SetSearchExhausted();
      void OnTileDirty(csg::Point3 const& index);
      void EnableWorldWatcher(bool enabled);
      bool FindDirectPathToDestination(const PathFinderNode* from, PathFinderDst*& dst);
      void OnPathFinderDstChanged(PathFinderDst const& dst, const char* reason);
      void RebuildOpenHeuristics();
      bool CheckIfIdle() const;
      void ClearNodes();

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
   
      std::unique_ptr<boost::object_pool<PathFinderNode>>  _nodePool;
      core::Guard                   navgrid_guard_;
      std::vector<PathFinderNode*>   open_;
      csg::Cube3                    closedBounds_;
      std::unordered_map<csg::Point3, PathFinderNode*, csg::Point3::Hash>         closed_;
      std::unordered_set<csg::Point3, csg::Point3::Hash>         watching_tiles_;
      std::vector<csg::Point3f>     _directPathCandiate;
      mutable const char*           _lastIdleCheckResult;

      std::unordered_map<csg::Point3, PathFinderNode*, csg::Point3::Hash> _openLookup;
   
      std::unique_ptr<PathFinderSrc>               source_;
      mutable std::unordered_map<dm::ObjectId, std::unique_ptr<PathFinderDst>>  destinations_;

      phys::OctTree::MovementCostCb _addFn;
      PathFinderNode* _currentNode;
      float           _maxMod;
};

std::ostream& operator<<(std::ostream& o, const AStarPathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_A_STAR_PATH_FINDER_H

