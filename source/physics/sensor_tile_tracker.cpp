#include "radiant.h"
#include "radiant_stdutil.h"
#include "octtree.h"
#include "collision_tracker.h"
#include "nav_grid_tile.h"
#include "sensor_tracker.h"
#include "sensor_tile_tracker.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/sensor.ridl.h"
#include "om/components/mob.ridl.h"

using namespace radiant;
using namespace radiant::phys;

#define ST_LOG(level)              LOG(physics.sensor_tracker, level) << "[" << _sensorTracker.GetLogPrefix() << " @ index " << _index << "] "

/* 
 * -- FlagsToString
 *
 * Convert a set of NavGridTile::ChangeNotifications flags to a friendly string
 */
static std::string FlagsToString(int flags)
{
   bool first = true;
   std::ostringstream s;

#define ADD_FLAG(name)              \
   if (flags & NavGridTile::name) { \
      if (!first) {                 \
         s << " ";                  \
      }                             \
      s << #name;                   \
      first = false;                \
   }

   ADD_FLAG(ENTITY_ADDED)
   ADD_FLAG(ENTITY_MOVED)
   ADD_FLAG(ENTITY_REMOVED)

#undef ADD_FLAG
   return s.str();
}

/* 
 * -- SensorTileTracker::SensorTileTracker
 *
 * Construct a new SensorTileTracker.  Registers for change callbacks on the NavGridTile
 * at `index`.
 */
SensorTileTracker::SensorTileTracker(SensorTracker& sensorTracker, csg::Point3 const& index, int flags) :
   _index(index),
   _sensorTracker(sensorTracker),
   _flags(0)
{
   ST_LOG(8) << "installing new tracker at " << index << "(flags: " << FlagsToString(flags) << ")";

   UpdateFlags(flags);
}

/* 
 * -- SensorTileTracker::~SensorTileTracker
 *
 * Simply removes all the entities from this tracker
 */
SensorTileTracker::~SensorTileTracker()
{
   ST_LOG(5) << "destroying sensor tile tracker at " << _index;
}

/* 
 * -- SensorTileTracker::UpdateFlags
 *
 * Notification from the SensorTracker that the flags for this SensorTileTracker have
 * changed.  In response, we update the callback flags on our NavGridTile and recompute
 * the set of overlapping Entities.
 *
 * This method is called whenever the bounding box of the sensor changes as well!
 */

void SensorTileTracker::UpdateFlags(int flags)
{
   NavGrid& navgrid = _sensorTracker.GetNavGrid();
   bool flagsChanged = flags != _flags;
   _flags = flags;

   ST_LOG(8) << "setting flags to " << FlagsToString(_flags) << ")";

   if (flagsChanged) {
      // The flags have changed, so we definitely need to change what notifications we're
      // listening on.
      NavGridTile& tile = navgrid.GridTileNonResident(_index);

      _ngtChangeGuard = tile.RegisterChangeCb([this](NavGridTile::ChangeNotification const& n) {
         CheckEntity(n.reason, n.entityId, n.tracker ? n.tracker->GetEntity() : nullptr);
      });

   } else if ((_flags & NavGridTile::ENTITY_MOVED) == 0) {
      // The flags not changed and we don't care about ENTITY_MOVED.  In this case, the
      // set of entities we're tracking hasn't changed, so don't go through the hassle of
      // checking the entire list against all the trackers on the tile!
      ST_LOG(8) << "overlapping entity set must be the same.  returning.";
      return;
   }

   // Loop through every tracker on this tile and figure out if it overlaps our sensor bounds.
   NavGridTile& tile = navgrid.GridTileNonResident(_index);
   tile.ForEachTracker([this](CollisionTrackerPtr tracker) {
      om::EntityPtr entity = tracker->GetEntity();
      dm::ObjectId entityId = entity->GetObjectId();
      CheckEntity(NavGridTile::ENTITY_MOVED, entityId, entity);
      return false;     // keep iterating...
   });
}

/* 
 * -- SensorTileTracker::CheckEntity
 *
 * Check to see if an entity is inside/outside the sensor.  If we only care about adds and
 * this is an add notification, the entity definitely is!  Otherwise, we just don't know and
 * have to ask the SensorTracker.
 */
void SensorTileTracker::CheckEntity(NavGridTile::ChangeNotifications reason, dm::ObjectId entityId, om::EntityPtr entity)
{
   ST_LOG(8) << "got change notification (entityId:" << entityId << " reason:" << FlagsToString(reason) << " flags:" << FlagsToString(_flags) << ")";
   if ((reason & (NavGridTile::ENTITY_ADDED | NavGridTile::ENTITY_MOVED)) != 0 &&
       (_flags & NavGridTile::ENTITY_MOVED) == 0) {
          ASSERT(entity);
          _sensorTracker.AddEntity(entityId, entity);
          return;
   }
   _sensorTracker.CheckEntity(entityId, entity);
}
