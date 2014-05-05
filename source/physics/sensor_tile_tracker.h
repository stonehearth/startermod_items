#ifndef _RADIANT_PHYSICS_SENSOR_TILE_TRACKER_H
#define _RADIANT_PHYSICS_SENSOR_TILE_TRACKER_H

#include "namespace.h"
#include "csg/cube.h"
#include "om/om.h"
#include <boost/container/flat_map.hpp>

BEGIN_RADIANT_PHYSICS_NAMESPACE

/* 
 * -- SensorTileTracker
 *
 * Tracks which entities overlap the sensor for a particular NavGridTile, and
 * communicates that back to the SensorTracker for updating the sensor contents.
 */
class SensorTileTracker {
public:
   SensorTileTracker(SensorTracker& sensorTracker, csg::Point3 const& index, int flags);
   virtual ~SensorTileTracker();

   bool ContainsEntity(dm::ObjectId entityId);
   void UpdateFlags(int flags);

private:
   void OnTileChanged(NavGridTile::ChangeNotification const& n);
   void OnTrackerAdded(CollisionTrackerPtr tracker);
   void OnTrackerChanged(CollisionTrackerPtr tracker);
   void OnEntityAdded(dm::ObjectId entityId, om::EntityPtr entity);
   void OnEntityRemoved(dm::ObjectId entityId);
   void RemoveAllEntities();

private:
   typedef boost::container::flat_map<dm::ObjectId, om::EntityRef>   EntityFlatMap;

private:
   SensorTracker& _sensorTracker;
   core::Guard    _ngtChangeGuard;
   int            _flags;
   csg::Point3    _index;
   EntityFlatMap  _entities;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_SENSOR_TILE_TRACKER_H
