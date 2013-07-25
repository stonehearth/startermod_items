#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

#include "math3d.h"
#include "math3d/common/color.h"
#include "math3d_collision.h"
#include "om/om.h"
#include "job.h"
#include "physics/namespace.h"
#include "radiant.pb.h"
#include "path.h"
#include "csg/point.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class PathFinder;
class Simulation;

class PathFinderEndpoint {
public:
   PathFinderEndpoint(PathFinder& pf, om::EntityRef entity);
   void AddAdjacentToOpenSet(std::vector<math3d::ipoint3>& open);
   math3d::ipoint3 GetPointInRegionAdjacentTo(math3d::ipoint3 const& adjacent) const;

   int EstimateMovementCost(const math3d::ipoint3& start) const;
   om::EntityPtr GetEntity() const { return entity_.lock(); }

private:
   int EstimateMovementCost(csg::Point3 const& start, csg::Point3 const& end) const;

public:
   PathFinder&       pf_;
   om::EntityRef     entity_;
   dm::Guard         guards_;
};

class PathFinder : public Job {
   public:
      PathFinder(lua_State* L, std::string name, om::EntityRef e, luabind::object solved, luabind::object dst_filter);
      virtual ~PathFinder();

      void AddDestination(om::EntityRef dst);
      void RemoveDestination(dm::ObjectId id);
      void SetSolvedCb(luabind::object solved);
      void SetFilterFn(luabind::object dst_filter);

      PathPtr GetSolution() const;

      void Restart();
      void SetReverseSearch(bool reversed);
      int EstimateCostToSolution();
      std::ostream& Format(std::ostream& o) const;


   public: // Job Interface
      bool IsIdle() const override;
      bool IsFinished() const override { return false; }
      void Work(const platform::timer &timer) override;
      void LogProgress(std::ostream&) const override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;

   private:
      friend PathFinderEndpoint;
      void RestartSearch();

   private:
      bool CompareEntries(const math3d::ipoint3 &a, const math3d::ipoint3 &b);
      void RecommendBestPath(std::vector<math3d::ipoint3> &points) const;
      int EstimateCostToDestination(const math3d::ipoint3 &pt) const;
      int EstimateCostToDestination(const math3d::ipoint3 &pt, PathFinderEndpoint** closest) const;

      math3d::ipoint3 GetFirstOpen();
      void ReconstructPath(std::vector<math3d::ipoint3> &solution, const math3d::ipoint3 &dst) const;
      void AddEdge(const math3d::ipoint3 &current, const math3d::ipoint3 &next, int cost);
      void RebuildHeap();

      void SolveSearch(const math3d::ipoint3& last, PathFinderEndpoint* dst);

   public:
      om::EntityRef                                entity_;
      om::MobRef                                   mob_;
      luabind::object                              solved_cb_;
      luabind::object                              dst_filter_;
      int                                          costToDestination_;
      bool                                         rebuildHeap_;
      bool                                         restart_search_;
      bool                                         search_exhausted_;
      bool                                         reversed_search_;
      mutable PathPtr                              solution_;
      std::vector<math3d::ipoint3>                      open_;
      std::unordered_map<math3d::ipoint3, bool, math3d::ipoint3::hash> closed_;
      std::hash_map<math3d::ipoint3, int>               f_;
      std::hash_map<math3d::ipoint3, int>               g_;
      std::hash_map<math3d::ipoint3, int>               h_;
      std::hash_map<math3d::ipoint3, math3d::ipoint3>   cameFrom_;

      PathFinderEndpoint                                 source_;
      mutable std::unordered_map<om::EntityId, std::unique_ptr<PathFinderEndpoint>>  destinations_;
};

typedef std::weak_ptr<PathFinder> PathFinderRef;
typedef std::shared_ptr<PathFinder> PathFinderPtr;

std::ostream& operator<<(std::ostream& o, const PathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

