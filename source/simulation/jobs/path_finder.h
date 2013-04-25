#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

#include "math3d.h"
#include "math3d/common/color.h"
#include "math3d_collision.h"
#include "job.h"
#include "destination.h"
#include "physics/namespace.h"
#include "radiant.pb.h"
#include "path.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class Simulation;

class PathFinder : public Job {
   public:
      PathFinder(std::string name, bool ownsDest);
      virtual ~PathFinder();

      void AddDestination(DestinationPtr dst);
      void RemoveDestination(om::EntityId id);

      enum State {
         CONSTRUCTED,
         RUNNING,
         SOLVED,
         EXHAUSTED,
         RESTARTING,
      };

      State GetState() const { return state_; }
      PathPtr GetSolution() const;

      void Restart();
      void Start(om::EntityRef entity, const math3d::ipoint3& start);
      int EstimateCostToSolution();
      std::ostream& Format(std::ostream& o) const;

   public: // Job Interface
      bool IsIdle(int now) const override;
      bool IsFinished() const override { return false; }
      void Work(int now, const platform::timer &timer) override;
      void LogProgress(std::ostream&) const override;
      void EncodeDebugShapes(protocol::shapelist *msg) const override;

   private:
      bool IsRunning() const;
      bool VerifyDestinationModifyTimes();
      bool CompareEntries(const math3d::ipoint3 &a, const math3d::ipoint3 &b);
      void RecommendBestPath(PointList &points) const;
      int EstimateCostToDestination(const math3d::ipoint3 &pt) const;
      int EstimateCostToDestination(const math3d::ipoint3 &pt, DestinationPtr& closest) const;

      math3d::ipoint3 GetFirstOpen();
      void ReconstructPath(PointList &solution, const math3d::ipoint3 &dst) const;
      void AddEdge(const math3d::ipoint3 &current, const math3d::ipoint3 &next, int cost);
      void RebuildHeap();

      void AbortSearch(State next, std::string reason);
      void SolveSearch(const math3d::ipoint3& last, DestinationPtr dst);
      void CheckSolution() const;

   public:
      mutable State                                state_;
      om::EntityRef                                entity_;
      math3d::ipoint3                              start_;
      int                                          startTime_;
      int                                          costToDestination_;
      bool                                         rebuildHeap_;
      bool                                         ownsDst_;
      mutable PathPtr                              solution_;
      int                                          solutionTime_;
      std::string                                  stopReason_;
      std::vector<math3d::ipoint3>                      open_;
      std::vector<math3d::ipoint3>                      closed_;
      std::hash_map<math3d::ipoint3, int>               f_;
      std::hash_map<math3d::ipoint3, int>               g_;
      std::hash_map<math3d::ipoint3, int>               h_;
      std::hash_map<math3d::ipoint3, math3d::ipoint3>   cameFrom_;
      mutable std::unordered_map<om::EntityId, DestinationPtr>  destinations_;
};

typedef std::weak_ptr<PathFinder> PathFinderRef;
typedef std::shared_ptr<PathFinder> PathFinderPtr;

std::ostream& operator<<(std::ostream& o, const PathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

