#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

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
   void AddAdjacentToOpenSet(std::vector<csg::Point3>& open);
   csg::Point3 GetPointInRegionAdjacentTo(csg::Point3 const& adjacent) const;

   int EstimateMovementCost(const csg::Point3& start) const;
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
      PathFinder(lua_State* L, std::string name);
      virtual ~PathFinder();

      void AddDestination(om::EntityRef dst);
      void RemoveDestination(dm::ObjectId id);
      void SetSource(om::EntityRef e);
      void SetSolvedCb(luabind::object solved);
      void SetFilterFn(luabind::object dst_filter);

      PathPtr GetSolution() const;

      void Restart();
      void Start();
      void Stop();
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
      bool CompareEntries(const csg::Point3 &a, const csg::Point3 &b);
      void RecommendBestPath(std::vector<csg::Point3> &points) const;
      int EstimateCostToDestination(const csg::Point3 &pt) const;
      int EstimateCostToDestination(const csg::Point3 &pt, PathFinderEndpoint** closest) const;

      csg::Point3 GetFirstOpen();
      void ReconstructPath(std::vector<csg::Point3> &solution, const csg::Point3 &dst) const;
      void AddEdge(const csg::Point3 &current, const csg::Point3 &next, int cost);
      void RebuildHeap();

      void SolveSearch(const csg::Point3& last, PathFinderEndpoint* dst);

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
      bool                                         enabled_;
      mutable PathPtr                              solution_;
      std::vector<csg::Point3>                      open_;

      std::unordered_map<csg::Point3, bool, csg::Point3::Hash> closed_;
      std::unordered_map<csg::Point3, int, csg::Point3::Hash>  f_;
      std::unordered_map<csg::Point3, int, csg::Point3::Hash>  g_;
      std::unordered_map<csg::Point3, int, csg::Point3::Hash>  h_;
      std::unordered_map<csg::Point3, csg::Point3, csg::Point3::Hash>  cameFrom_;

      std::unique_ptr<PathFinderEndpoint>          source_;
      mutable std::unordered_map<dm::ObjectId, std::unique_ptr<PathFinderEndpoint>>  destinations_;
};

typedef std::weak_ptr<PathFinder> PathFinderRef;
typedef std::shared_ptr<PathFinder> PathFinderPtr;

std::ostream& operator<<(std::ostream& o, const PathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

