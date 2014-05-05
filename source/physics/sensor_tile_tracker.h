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

   void UpdateFlags(int flags);

private:
   SensorTracker& _sensorTracker;
   core::Guard    _ngtChangeGuard;
   int            _flags;
   csg::Point3    _index;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_SENSOR_TILE_TRACKER_H
