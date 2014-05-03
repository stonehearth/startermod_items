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

SensorTileTracker::~SensorTileTracker()
{
   ST_LOG(5) << "destroying sensor tile tracker at " << _index;
   RemoveAllEntities();
}

void SensorTileTracker::UpdateFlags(int flags)
{
   // If the flags haven't changed and the move flag is *not* set, we don't have to craw
   // everything.
   if (flags == _flags && ((_flags & NavGridTile::ENTITY_MOVED) == 0)) {
      ST_LOG(8) << "current flags == new flags (" << FlagsToString(flags) << ").  nop.";
   } else {
      ST_LOG(8) << "setting flags to " << FlagsToString(_flags) << ")";
      _flags = flags;

      EntityFlatMap entitiesToRemove = std::move(_entities);
      ASSERT(_entities.empty());

      NavGridTile& tile = _sensorTracker.GetNavGrid().GridTileNonResident(_index);
      tile.ForEachTracker([this, &entitiesToRemove](CollisionTrackerPtr tracker) {
         csg::Cube3 const& bounds = _sensorTracker.GetBounds();
         om::EntityPtr entity = tracker->GetEntity();
         if (entity) {
            if (tracker->Intersects(bounds)) {
               ST_LOG(7) << "in init sensor tile " << *entity << " overlaps " << bounds << " (~" << tracker->GetOverlappingRegion(bounds).GetBounds() << ")";

               dm::ObjectId entityId = entity->GetObjectId();
               entitiesToRemove.erase(entityId);
               _entities[entityId] = entity;
            } else {
               ST_LOG(7) << "in init sensor tile " << *entity << " does not overlap " << bounds << ".";
            }
         } else {
            ST_LOG(7) << "got invalid entity in sensor tracker.  seriously?";
         }
         return true;
      });

      for (auto const& entry : entitiesToRemove) {
         ST_LOG(7) << "in init sensor tile, removing " << entry.first << " from sensor";
         _sensorTracker.TryRemoveEntityFromSensor(entry.first);
      }
      for (auto const& entry : _entities) {
         ST_LOG(7) << "in init sensor tile, adding " << entry.first << " to sensor";
         _sensorTracker.TryAddEntityToSensor(entry.first, entry.second);
      }
   }
}

bool SensorTileTracker::ContainsEntity(dm::ObjectId entityId)
{
   return _entities.find(entityId) != _entities.end();
}

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
   _entities[entityId] = entity;
   _sensorTracker.TryAddEntityToSensor(entityId, entity);
}

void SensorTileTracker::OnEntityRemoved(dm::ObjectId entityId)
{
   csg::Cube3 const& bounds = _sensorTracker.GetBounds();

   NavGridTile& tile = _sensorTracker.GetNavGrid().GridTileNonResident(_index);
   bool remove = tile.ForEachTrackerForEntity(entityId, [this, &bounds](CollisionTrackerPtr tracker) {
      // keep iterating until you find something that intersects the bounds.
      return !tracker->Intersects(bounds);
   });

   if (remove) {
      ST_LOG(7) << entityId << " no longer overlaps " << bounds << " in any collision tracker.  try removing.";
      _entities.erase(entityId);
      _sensorTracker.TryRemoveEntityFromSensor(entityId);
   } else {
      ST_LOG(7) << entityId << " still overlaps " << bounds << " in some collision tracker.  not trying to remove.";
   }
}

void SensorTileTracker::RemoveAllEntities()
{
   for (const auto& entry : _entities) {
      _sensorTracker.TryRemoveEntityFromSensor(entry.first);
   }
}
