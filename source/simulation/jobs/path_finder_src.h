#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_SRC_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_SRC_H

#include "om/om.h"
#include "job.h"
#include "physics/namespace.h"
#include "protocols/radiant.pb.h"
#include "path.h"
#include "csg/point.h"
#include "om/region.h"
#include "path_finder_node.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class PathFinder;

class PathFinderSrc {
public:
   PathFinderSrc(PathFinder& pf, om::EntityRef entity);
   ~PathFinderSrc();

   bool IsIdle() const;
   void InitializeOpenSet(std::vector<PathFinderNode>& open);
   void EncodeDebugShapes(protocol::shapelist *msg) const;
   void SetSourceOverride(csg::Point3 const& location);

protected:
   Simulation& GetSim() const { return pf_.GetSim(); }

public:
   PathFinder&                pf_;
   om::EntityRef              entity_;
   bool                       moving_;
   core::Guard                guards_;
   dm::TracePtr               transform_trace_;
   dm::TracePtr               moving_trace_;
   phys::TerrainChangeCbId    collision_cb_id_;
   csg::Point3                source_override_;
   bool                       use_source_override_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_SRC_H

