#include "radiant.h"
#include "radiant_stdutil.h"
#include "octtree.h"
#include "collision_tracker.h"
#include "sensor_tracker.h"
#include "sensor_tile_tracker.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/sensor.ridl.h"
#include "om/components/mob.ridl.h"

using namespace radiant;
using namespace radiant::phys;

#define ST_LOG(level)              LOG(physics.sensor_tracker, level) << "[" << GetLogPrefix() << "] "

#define ST_LOG_SENSOR_CONTENTS(level, contents) \
   if (LOG_IS_ENABLED(physics.sensor_tracker, level)) { \
      for (auto const& entry : contents) { \
         om::EntityPtr entity = entry.second.lock(); \
         if (entity) { \
            ST_LOG(level) << "   " << entry.first << " -> " << *entity; \
         } \
      } \
   }


/* 
 * -- SensorTracker::SensorTracker
 *
 * Nothing to see here, move along...
 */
SensorTracker::SensorTracker(NavGrid& navgrid, om::EntityPtr entity, om::SensorPtr sensor) :
   navgrid_(navgrid),
   entity_(entity),
   sensor_(sensor),
   bounds_(csg::Point3::zero, csg::Point3::zero),
   last_bounds_(csg::Point3::zero, csg::Point3::zero)
{
}

/* 
 * -- SensorTracker::Initialize
 *
 */
void SensorTracker::Initialize()
{
   om::SensorPtr sensor = sensor_.lock();
   om::EntityPtr entity = entity_.lock();
   if (sensor && entity) {
      ST_LOG(3) << "creating new sensor tracker";

      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         mob_ = mob;
         mob_trace_ = mob->TraceTransform("sensor tracker", navgrid_.GetTraceCategory())
            ->OnModified([this]() {
               OnSensorMoved();
            })
            ->PushObjectState();
      }
   }
}

/* 
 * -- SensorTracker::OnSensorMoved
 *
 */
void SensorTracker::OnSensorMoved()
{
   om::MobPtr mob = mob_.lock();
   om::SensorPtr sensor = sensor_.lock();
   if (sensor && mob) {
      csg::Point3 location = mob->GetWorldGridLocation();
      bounds_ = sensor->GetCube().Translated(location);

      ST_LOG(3) << "sensor moved to " << location << ".  bounds are now " << bounds_;
      TraceNavGridTiles();
   }
}

NavGrid& SensorTracker::GetNavGrid() const
{
   return navgrid_;
}

csg::Cube3 const& SensorTracker::GetBounds() const
{
   return bounds_;
}

void SensorTracker::TraceNavGridTiles()
{
   ST_LOG(5) << "tracking tiles (bounds: " << bounds_ << " last_bounds: " << last_bounds_ << ")";

   csg::Cube3 current = csg::GetChunkIndex(bounds_, TILE_SIZE);
   csg::Cube3 previous = csg::GetChunkIndex(last_bounds_, TILE_SIZE);

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : previous) {
      if (!current.Contains(cursor)) {
         _sensorTileTrackers.erase(cursor);
      }
   }

   csg::Cube3 inner = csg::Cube3::Construct(current.min + csg::Point3(1, 0, 1),
                                            current.max - csg::Point3(1, 0, 1));

   for (csg::Point3 const& cursor : current) {
      // We're on the inner ring.  Need to subscribe to remove events.
      // We're on the fringe.  Need to subscribe to move events.
      int flags = inner.Contains(cursor) ? NavGridTile::ENTITY_ADDED : NavGridTile::ENTITY_MOVED;
      flags |= NavGridTile::ENTITY_REMOVED;

      auto i = _sensorTileTrackers.find(cursor);
      if (i != _sensorTileTrackers.end()) {
         i->second->UpdateFlags(flags);
      } else {
         _sensorTileTrackers[cursor] = std::make_shared<SensorTileTracker>(*this, cursor, flags);
      }
   }
}

void SensorTracker::TryRemoveEntityFromSensor(dm::ObjectId entityId)
{
   auto sensor = sensor_.lock();
   if (sensor) {
      auto& contents = sensor->GetContainer();
      if (contents.find(entityId) != contents.end()) {
         // xxx: to avoid this iteration, we could keep a count of the number of tiles which have an entity >_<
         // or a backmap of entity ids to tiles which have that entity...
         for (auto const& entry: _sensorTileTrackers) {
            if (entry.second->ContainsEntity(entityId)) {
               ST_LOG(7) << "entity " << entityId << " still in the tile_map.  not removing from sensor.";
               return;
            }
         }
         ST_LOG(7) << "entity " << entityId << " eradicated from the tile_map.  removing from sensor.";
         contents.Remove(entityId);
         ST_LOG_SENSOR_CONTENTS(9, contents)
      }
   }
}

void SensorTracker::TryAddEntityToSensor(dm::ObjectId entityId, om::EntityRef e)
{
   auto sensor = sensor_.lock();
   if (sensor) {
      auto& contents = sensor->GetContainer();
      if (contents.find(entityId) == contents.end()) {
         ST_LOG(7) << "entity " << *e.lock() << " first added to tile map.  adding to sensor!.";
         contents.Add(entityId, e);
         ST_LOG_SENSOR_CONTENTS(9, contents)
      } else {
         ST_LOG(7) << "entity " << *e.lock() << " already in the tile_map.  not adding to sensor.";
      }
   }
}

std::string SensorTracker::GetLogPrefix() const
{
   om::SensorPtr sensor = sensor_.lock();
   om::EntityPtr entity = entity_.lock();
   std::string entityName = "expired";
   if (entity) {
      entityName = BUILD_STRING(*entity);
   }
   return BUILD_STRING("sensor " << (sensor ? sensor->GetName() : std::string("expired")) << " for " << entityName);
}
