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

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class PathFinder;
class PathFinderSrc;
class PathFinderDst;
class Simulation;

class PathFinder : public Job {
   public:
      PathFinder(Simulation& sim, std::string name);
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
      int EstimateCostToSolution();
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
      void RestartSearch(const char* reason);
      bool IsSearchExhausted() const;

   private:
      bool CompareEntries(const csg::Point3 &a, const csg::Point3 &b);
      void RecommendBestPath(std::vector<csg::Point3> &points) const;
      int EstimateCostToDestination(const csg::Point3 &pt) const;
      int EstimateCostToDestination(const csg::Point3 &pt, PathFinderDst** closest) const;

      csg::Point3 GetFirstOpen();
      void ReconstructPath(std::vector<csg::Point3> &solution, const csg::Point3 &dst) const;
      void AddEdge(const csg::Point3 &current, const csg::Point3 &next, int cost);
      void RebuildHeap();

      void SolveSearch(const csg::Point3& last, PathFinderDst* dst);
      csg::Point3 GetSourceLocation();

   public:
      om::EntityRef                                entity_;
      om::MobRef                                   mob_;
      luabind::object                              solved_cb_;
      luabind::object                              dst_filter_;
      int                                          costToDestination_;
      bool                                         rebuildHeap_;
      bool                                         restart_search_;
      bool                                         enabled_;
      mutable PathPtr                              solution_;
      csg::Color4                                  debug_color_;
   
      std::vector<csg::Point3>                      open_;

      std::unordered_map<csg::Point3, bool, csg::Point3::Hash> closed_;
      std::unordered_map<csg::Point3, int, csg::Point3::Hash>  f_;
      std::unordered_map<csg::Point3, int, csg::Point3::Hash>  g_;
      std::unordered_map<csg::Point3, int, csg::Point3::Hash>  h_;
      std::unordered_map<csg::Point3, csg::Point3, csg::Point3::Hash>  cameFrom_;
   
      std::unique_ptr<PathFinderSrc>               source_;
      mutable std::unordered_map<dm::ObjectId, std::unique_ptr<PathFinderDst>>  destinations_;
};

typedef std::weak_ptr<PathFinder> PathFinderRef;
typedef std::shared_ptr<PathFinder> PathFinderPtr;

std::ostream& operator<<(std::ostream& o, const PathFinder& pf);

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

