#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_DST_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_DST_H

#include "om/om.h"
#include "job.h"
#include "protocols/radiant.pb.h"
#include "path.h"
#include "csg/point.h"
#include "om/region.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class PathFinder;
class Simulation;

class PathFinderDst {
public:
   typedef std::function<void(const char *reason)> ChangedCb;

   PathFinderDst(Simulation& sim, om::EntityRef e, std::string const& name, ChangedCb changed_cb);
   ~PathFinderDst();

   om::EntityPtr GetEntity() const { return entity_.lock(); }
   
   bool IsIdle() const;
   dm::ObjectId GetEntityId() const;
   csg::Point3 GetPointOfInterest(csg::Point3 const& adjacent) const;
   int EstimateMovementCost(const csg::Point3& start) const;
   void EncodeDebugShapes(protocol::shapelist *msg, csg::Color4 const& debug_color) const;
   
private:
   void CreateTraces();
   void DestroyTraces();
   void ClipAdjacentToTerrain();
   int EstimateMovementCost(csg::Point3 const& start, csg::Point3 const& end) const;

public:
   Simulation&                sim_;
   std::string                name_;
   dm::ObjectId               id_;
   om::EntityRef              entity_;
   ChangedCb                  changed_cb_;
   bool                       moving_;
   om::DeepRegionGuardPtr     region_guard_;
   csg::Region3               world_space_adjacent_region_;
   dm::TracePtr               moving_trace_;
   dm::TracePtr               transform_trace_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

