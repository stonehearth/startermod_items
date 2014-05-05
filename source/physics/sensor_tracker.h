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
 * Tracks individual Sensors for Entities in the world.  Most of the work is done by
 * the SensorTileTracker, which tracks changes to individual NavGrid tiles.  The
 * SensorTracker just manages the lifetime aof the SensorTrackerTiles and arbitrates
 * access to the actual sensor data.
 */
class SensorTracker : public std::enable_shared_from_this<SensorTracker> {
public:
   SensorTracker(NavGrid& navgrid, om::EntityPtr entity, om::SensorPtr sensor);
   virtual ~SensorTracker() { }
   
   void Initialize();
   std::string GetLogPrefix() const;
   NavGrid& GetNavGrid() const;

   void CheckEntity(dm::ObjectId entityId, om::EntityRef e);
   void AddEntity(dm::ObjectId entityId, om::EntityRef e);

private:
   void OnSensorMoved();
   void TraceNavGridTiles();

private:
   typedef std::unordered_map<csg::Point3, SensorTileTrackerPtr, csg::Point3::Hash>    SensorTileTrackers;
   typedef std::unordered_map<dm::ObjectId, int> EntitiesTrackedMap;
private:
   NavGrid&          navgrid_;
   csg::Cube3        last_bounds_;
   csg::Cube3        bounds_;
   om::EntityRef     entity_;
   om::SensorRef     sensor_;
   om::MobRef        mob_;
   dm::TracePtr      mob_trace_;
   SensorTileTrackers  _sensorTileTrackers;
   EntitiesTrackedMap  _entitiesTracked;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_SENSOR_TRACKER_H
