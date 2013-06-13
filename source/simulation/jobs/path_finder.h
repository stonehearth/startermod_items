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

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class Simulation;

class PathFinder : public Job {
   public:
      PathFinder(std::string name, om::EntityRef e, luabind::object solved, luabind::object dst_filter);
      virtual ~PathFinder();

      void AddDestination(om::DestinationRef dst);
      void RemoveDestination(om::DestinationRef dst);

      PathPtr GetSolution() const;

      void Restart();
      int EstimateCostToSolution();
      std::ostream& Format(std::ostream& o) const;

   public: // Job Interface
      bool IsIdle(int now) const override;
      bool IsFinished() const override { return false; }
      void Work(int now, const platform::timer &timer) override;
      void LogProgress(std::ostream&) const override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;

   private:
      bool CompareEntries(const math3d::ipoint3 &a, const math3d::ipoint3 &b);
      void RecommendBestPath(std::vector<math3d::ipoint3> &points) const;
      int EstimateMovementCost(const math3d::ipoint3& start, om::DestinationPtr dst) const;
      int EstimateCostToDestination(const math3d::ipoint3 &pt) const;
      int EstimateCostToDestination(const math3d::ipoint3 &pt, om::DestinationPtr& closest) const;

      math3d::ipoint3 GetFirstOpen();
      void ReconstructPath(std::vector<math3d::ipoint3> &solution, const math3d::ipoint3 &dst) const;
      void AddEdge(const math3d::ipoint3 &current, const math3d::ipoint3 &next, int cost);
      void RebuildHeap();

      void SolveSearch(const math3d::ipoint3& last, om::DestinationPtr dst);

   public:
      om::EntityRef                                entity_;
      luabind::object                              solved_cb_;
      luabind::object                              dst_filter_;
      int                                          costToDestination_;
      bool                                         rebuildHeap_;
      bool                                         restart_search_;
      bool                                         search_exhausted_;
      mutable PathPtr                              solution_;
      dm::GuardSet                                 guards_;
      std::vector<math3d::ipoint3>                      open_;
      std::vector<math3d::ipoint3>                      closed_;
      std::hash_map<math3d::ipoint3, int>               f_;
      std::hash_map<math3d::ipoint3, int>               g_;
      std::hash_map<math3d::ipoint3, int>               h_;
      std::hash_map<math3d::ipoint3, math3d::ipoint3>   cameFrom_;
      mutable std::unordered_map<om::EntityId, om::DestinationRef>  destinations_;
};

typedef std::weak_ptr<PathFinder> PathFinderRef;
typedef std::shared_ptr<PathFinder> PathFinderPtr;

std::ostream& operator<<(std::ostream& o, const PathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

