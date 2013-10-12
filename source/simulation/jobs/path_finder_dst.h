#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_DST_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_DST_H

#include "om/om.h"
#include "job.h"
#include "radiant.pb.h"
#include "path.h"
#include "csg/point.h"
#include "om/region.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Path;
class PathFinder;
class Simulation;

class PathFinderDst {
public:
   PathFinderDst(PathFinder& pf, om::EntityRef entity);
   ~PathFinderDst();

   om::EntityPtr GetEntity() const { return entity_.lock(); }
   bool IsIdle() const;

   csg::Point3 GetPointfInterest(csg::Point3 const& adjacent) const;
   int EstimateMovementCost(const csg::Point3& start) const;
   void EncodeDebugShapes(protocol::shapelist *msg, csg::Color4 const& debug_color) const;
   
private:
   void ClipAdjacentToTerrain();
   int EstimateMovementCost(csg::Point3 const& start, csg::Point3 const& end) const;

public:
   PathFinder&                pf_;
   core::Guard                  guards_;
   om::EntityRef              entity_;
   bool                       moving_;
   om::DeepRegionGuardPtr     region_guard_;
   phys::TerrainChangeCbId    collision_cb_id_;
   csg::Region3               world_space_adjacent_region_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_H

