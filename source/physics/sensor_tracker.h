#ifndef _RADIANT_PHYSICS_SENSOR_TRACKER_H
#define _RADIANT_PHYSICS_SENSOR_TRACKER_H

#include "namespace.h"
#include "csg/cube.h"
#include "om/om.h"
#include "core/guard.h"
#include <unordered_map>
#include <boost/container/flat_map.hpp>

BEGIN_RADIANT_PHYSICS_NAMESPACE

/* 
 * -- SensorTracker
 *
 */
class SensorTracker : public std::enable_shared_from_this<SensorTracker> {
public:
   SensorTracker(OctTree& octree, om::EntityPtr entity, om::SensorPtr sensor);
   virtual ~SensorTracker() { }
   
   void Initialize();

private:
   void OnSensorMoved();
   void TraceNavGridTiles();
   void TraceNavGridTile(csg::Point3 const& index, int flags);
   void RemoveAllEntitiesFrom(csg::Point3 const& index);
   void OnEntityChanged(csg::Point3 const& index, int f, dm::ObjectId entityId, om::EntityPtr entity);
   void OnEntityRemovedFromTile(csg::Point3 const& index, om::Sensor& s, dm::ObjectId entityId);
   void OnEntityAddedToTile(csg::Point3 const& index, om::Sensor& s, dm::ObjectId entityId, om::EntityRef e);
   void TryRemoveEntityFromSensor(dm::ObjectId entityId); 
   void TryAddEntityToSensor(dm::ObjectId entityId, om::EntityRef e);

private:
   typedef boost::container::flat_map<dm::ObjectId, om::EntityRef>   EntityFlatMap;

   struct NavGridChangeTracker {
      int            add_move_flag;
      core::Guard    guard;
      EntityFlatMap  entities;
      NavGridChangeTracker(NavGridChangeTracker&& other) : add_move_flag(other.add_move_flag), guard(std::move(other.guard)) { }
      NavGridChangeTracker(int f, core::Guard&& g) : add_move_flag(f), guard(std::move(g)) { }

   private:
      NO_COPY_CONSTRUCTOR(NavGridChangeTracker);
   };

   typedef std::unordered_map<csg::Point3, NavGridChangeTracker, csg::Point3::Hash>    ChangeTrackerMap;

private:
   OctTree&          octtree_;
   csg::Cube3        last_bounds_;
   csg::Cube3        bounds_;
   om::EntityRef     entity_;
   om::SensorRef     sensor_;
   om::MobRef        mob_;
   dm::TracePtr      mob_trace_;
   ChangeTrackerMap  tile_map_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_SENSOR_TRACKER_H
