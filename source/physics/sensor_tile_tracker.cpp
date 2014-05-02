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

#define ST_LOG(level)              LOG(physics.sensor_tracker, level) << "[" << _sensorTracker.GetLogPrefix() << " @ " << _index << "] "

SensorTileTracker::SensorTileTracker(SensorTracker& sensorTracker, csg::Point3 const& index, int flags) :
   _index(index),
   _sensorTracker(sensorTracker),
   _flags(0)
{
   ST_LOG(8) << "installing new tracker at " << index << "(flags: " << flags << ")";

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
   if (flags == _flags) {
      ST_LOG(8) << "current flags == new flags (" << flags << ").  nop.";
   } else {
      ST_LOG(8) << "setting flags to " << _flags << ")";
      _flags = flags;

      EntityFlatMap entitiesToRemove = std::move(_entities);
      ASSERT(_entities.empty());

      NavGridTile& tile = _sensorTracker.GetNavGrid().GridTileNonResident(_index);
      tile.ForEachTracker([this, &entitiesToRemove](CollisionTrackerPtr tracker) {
         om::EntityPtr entity = tracker->GetEntity();
         if (entity && tracker->Intersects(_sensorTracker.GetBounds())) {
            dm::ObjectId entityId = entity->GetObjectId();
            entitiesToRemove.erase(entityId);
            _entities[entityId] = entity;
         }
         return true;
      });

      for (auto const& entry : entitiesToRemove) {
         _sensorTracker.TryRemoveEntityFromSensor(entry.first);
      }
      for (auto const& entry : _entities) {
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
   ST_LOG(8) << "got change notification (entityId:" << n.entityId << " reason:" << n.reason << ")";
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
         ST_LOG(7) << "added" << *entity << " overlaps " << bounds << " (~" << tracker->GetOverlappingRegion(bounds).GetBounds() << ")";
         OnEntityAdded(entityId, entity);
      } else {
         ST_LOG(7) << "added" << *entity << " does not overlap " << bounds << ".  ignoring.";
      }
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
