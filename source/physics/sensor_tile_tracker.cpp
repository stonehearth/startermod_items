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

   NavGridTile& tile = _sensorTracker.GetNavGrid().GridTileNonResident(_index);
   _ngtChangeGuard = tile.RegisterChangeCb([this](NavGridTile::ChangeNotification const& n) {
      OnTileChanged(n);
   });
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
   RemoveAllEntities();
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
   bool flagsChanged = flags != _flags;
   _flags = flags;

   ST_LOG(8) << "setting flags to " << FlagsToString(_flags) << ")";

   if (flagsChanged) {
      // The flags have changed, so we definitely need to change what notifications we're
      // listening on.
      NavGridTile& tile = _sensorTracker.GetNavGrid().GridTileNonResident(_index);
      _ngtChangeGuard = tile.RegisterChangeCb([this](NavGridTile::ChangeNotification const& n) {
         OnTileChanged(n);
      });
   } else if ((_flags & NavGridTile::ENTITY_MOVED) == 0) {
      // The flags not changed and we don't care about ENTITY_MOVED.  In this case, the
      // set of entities we're tracking hasn't changed, so don't go through the hassle of
      // checking the entire list against all the trackers on the tile!
      ST_LOG(8) << "overlapping entity set must be the same.  returning.";
      return;
   }

   // Our `entities_` list is completely invalid.  Loop through every tracker on this tile
   // and figure out if it overlaps our sensor bounds.  Use that information to correct
   // the `entities_' list.
   NavGridTile& tile = _sensorTracker.GetNavGrid().GridTileNonResident(_index);
   tile.ForEachTracker([this](CollisionTrackerPtr tracker) {
      OnTrackerChanged(tracker);
      return true;
   });
}

/* 
 * -- SensorTileTracker::UpdateFlags
 *
 * Return whether or not this tracker believes the specified entity is within range of
 * the sensor.
 */
bool SensorTileTracker::ContainsEntity(dm::ObjectId entityId)
{
   return _entities.find(entityId) != _entities.end();
}

/* 
 * -- SensorTileTracker::OnTileChanged
 *
 * Notification from the NavGridTile that a tracker on the tile things an Entity
 * has changed position.  Update our own data accordingly.
 */
void SensorTileTracker::OnTileChanged(NavGridTile::ChangeNotification const& n)
{
   ST_LOG(8) << "got change notification (entityId:" << n.entityId << " reason:" << FlagsToString(n.reason) << " flags:" << FlagsToString(_flags) << ")";
   if ((n.reason & _flags) != 0) {
      switch (n.reason) {
      case NavGridTile::ENTITY_ADDED:
         OnTrackerAdded(n.tracker);
         break;
      case NavGridTile::ENTITY_MOVED:
         OnTrackerChanged(n.tracker);
         break;
      case NavGridTile::ENTITY_REMOVED:
         OnEntityRemoved(n.entityId);
         break;
      default:
         ASSERT(false);
      }
   }
}

/* 
 * -- SensorTileTracker::OnTrackerAdded
 *
 * Called when a tracker overlaps the sensor.
 */
void SensorTileTracker::OnTrackerAdded(CollisionTrackerPtr tracker)
{
   csg::Cube3 const& bounds = _sensorTracker.GetBounds();
   om::EntityPtr entity = tracker->GetEntity();
   if (entity) {
      dm::ObjectId entityId = entity->GetObjectId();
      if (tracker->Intersects(bounds)) {
         ST_LOG(7) << "added " << *entity << " overlaps " << bounds << " (~" << tracker->GetOverlappingRegion(bounds).GetBounds() << ")";
         OnEntityAdded(entityId, entity);
      } else {
         ST_LOG(7) << "added " << *entity << " does not overlap " << bounds << ".  ignoring.";
      }
   } else {
      ST_LOG(3) << "invalid entity!  why does this tracker still exists?";
   }
}

void SensorTileTracker::OnTrackerChanged(CollisionTrackerPtr tracker)
{
   csg::Cube3 const& bounds = _sensorTracker.GetBounds();
   om::EntityPtr entity = tracker->GetEntity();

   if (entity) {
      dm::ObjectId entityId = entity->GetObjectId();
      if (tracker->Intersects(bounds)) {
         ST_LOG(7) "moved " << *entity << " overlaps " << bounds << " (~" << tracker->GetOverlappingRegion(bounds).GetBounds() << ")";
         OnEntityAdded(entityId, entity);
      } else {
         ST_LOG(7) "moved " << *entity << " does not overlap " << bounds << ".  checking for removal...";
         OnEntityRemoved(entityId);
      }
   } else {
      ST_LOG(3) << "invalid entity!  why does this tracker still exists?";
   }
}

void SensorTileTracker::OnEntityAdded(dm::ObjectId entityId, om::EntityPtr entity)
{
   auto i = _entities.find(entityId);
   if (i == _entities.end()) {
      _entities[entityId] = entity;
      _sensorTracker.OnEntityAddedToSensorTileTracker(entityId, entity);
   } else {
      ST_LOG(7) << "ignoring OnEntityAdded cb for tracked entity " << entityId;
   }
}

void SensorTileTracker::OnEntityRemoved(dm::ObjectId entityId)
{
   csg::Cube3 const& bounds = _sensorTracker.GetBounds();

   auto i = _entities.find(entityId);
   if (i != _entities.end()) {
      NavGridTile& tile = _sensorTracker.GetNavGrid().GridTileNonResident(_index);
      bool remove = tile.ForEachTrackerForEntity(entityId, [this, &bounds](CollisionTrackerPtr tracker) {
         // keep iterating until you find something that intersects the bounds.
         return !tracker->Intersects(bounds);
      });

      if (remove) {
         ST_LOG(7) << entityId << " no longer overlaps " << bounds << " in any collision tracker.  try removing.";
         _entities.erase(i);
         _sensorTracker.OnEntityRemovedFromSensorTileTracker(entityId);
      } else {
         ST_LOG(7) << entityId << " still overlaps " << bounds << " in some collision tracker.  not trying to remove.";
      }
   } else {
      ST_LOG(7) << "ignoring OnEntityRemoved cb for untracked entity " << entityId;
   }
}

void SensorTileTracker::RemoveAllEntities()
{
   for (const auto& entry : _entities) {
      ST_LOG(7) << "removing entity " << entry.first << " in RemoveAllEntities";
      _sensorTracker.OnEntityRemovedFromSensorTileTracker(entry.first);
   }
   _entities.clear();
}
