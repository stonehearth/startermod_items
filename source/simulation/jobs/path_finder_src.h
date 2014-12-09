#ifndef _RADIANT_SIMULATION_JOBS_PATH_FINDER_SRC_H
#define _RADIANT_SIMULATION_JOBS_PATH_FINDER_SRC_H

#include <boost/pool/object_pool.hpp>
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
   typedef std::function<void(const char *reason)> ChangedCb;

   PathFinderSrc(om::EntityRef entity, std::string const& name, ChangedCb changed_cb);
   ~PathFinderSrc();

   bool IsIdle() const;
   void Start(std::vector<PathFinderNode*>& open, boost::object_pool<PathFinderNode> &nodePool);
   void EncodeDebugShapes(protocol::shapelist *msg) const;
   void SetSourceOverride(csg::Point3f const& location);
   csg::Point3f GetSourceLocation() const;

private:
   void InitializeOpenSet(std::vector<PathFinderNode*>& open, boost::object_pool<PathFinderNode> &nodePool);

public:
   std::string                name_;
   ChangedCb                  changed_cb_;
   om::EntityRef              entity_;
   core::Guard                guards_;
   dm::TracePtr               transform_trace_;
   phys::TerrainChangeCbId    collision_cb_id_;
   csg::Point3f               source_location_;
   csg::Point3f               source_override_;
   bool                       use_source_override_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_JOBS_PATH_FINDER_SRC_H

