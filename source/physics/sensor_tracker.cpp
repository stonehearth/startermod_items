#include "radiant.h"
#include "radiant_stdutil.h"
#include "octtree.h"
#include "collision_tracker.h"
#include "sensor_tracker.h"
#include "csg/util.h"
#include "om/entity.h"
#include "om/components/sensor.ridl.h"
#include "om/components/mob.ridl.h"

using namespace radiant;
using namespace radiant::phys;

#define ST_LOG(level)              LOG(physics.sensor_tracker, level) << "[sensor " << (sensor_.lock()->GetName()) << " for " << *entity_.lock() << "] "


/* 
 * -- SensorTracker::SensorTracker
 *
 * Nothing to see here, move along...
 */
SensorTracker::SensorTracker(OctTree& octtree, om::EntityPtr entity, om::SensorPtr sensor) :
   octtree_(octtree),
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
         mob_trace_ = mob->TraceTransform("sensor tracker", octtree_.GetNavGrid().GetTraceCategory())
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

void SensorTracker::TraceNavGridTiles()
{
   ST_LOG(5) << "tracking tiles (bounds: " << bounds_ << " last_bounds: " << last_bounds_ << ")";

   csg::Cube3 current = csg::GetChunkIndex(bounds_, TILE_SIZE);
   csg::Cube3 previous = csg::GetChunkIndex(last_bounds_, TILE_SIZE);

   // Remove trackers from tiles which no longer overlap the current bounds of the tracker,
   // but did overlap their previous bounds.
   for (csg::Point3 const& cursor : previous) {
      if (!current.Contains(cursor)) {
         RemoveAllEntitiesFrom(cursor);
      }
   }

   csg::Cube3 inner = csg::Cube3::Construct(current.min + csg::Point3(1, 0, 1),
                                            current.max - csg::Point3(1, 0, 1));

   for (csg::Point3 const& cursor : current) {
      if (!previous.Contains(cursor)) {
         int flag;
         if (inner.Contains(cursor)) {
            // We're on the inner ring.  Need to subscribe to remove events.
            flag = NavGridTile::ENTITY_ADDED;
         } else {
            // We're on the fringe.  Need to subscribe to move events.
            flag = NavGridTile::ENTITY_MOVED;
         }
         TraceNavGridTile(cursor, flag);
      }
   }
}

void SensorTracker::TraceNavGridTile(csg::Point3 const& index, int add_move_flag)
{
   auto i = tile_map_.find(index);
   if (i != tile_map_.end() && i->second.add_move_flag == add_move_flag) {
      ST_LOG(7) << "ignoring tile at " << index << ".  tracking data exists and is correct.";
      return; // already perfectly traced
   }
   
   ST_LOG(8) << "installing new tracker at " << index << "(add_move_flag: " << add_move_flag << ")";
   NavGridTile& tile = octtree_.GetNavGrid().GridTileNonResident(index);
   core::Guard guard = tile.TraceEntityPositions(add_move_flag | NavGridTile::ENTITY_REMOVED, bounds_, 
      [this, index](int reason, dm::ObjectId entityId, om::EntityPtr entity) {
         OnEntityChanged(index, reason, entityId, entity);
      });
   tile_map_.insert(std::make_pair(index,  NavGridChangeTracker(add_move_flag, std::move(guard))));

   ST_LOG(8) << "manually updating all entities at " << index << "(add_move_flag: " << add_move_flag << ")";
   tile.ForEachTracker([this, index, add_move_flag](CollisionTrackerPtr tracker) {
      om::EntityPtr entity = tracker->GetEntity();
      if (entity && tracker->Intersects(bounds_)) {
         OnEntityChanged(index, add_move_flag, entity->GetObjectId(), entity);
      } else {
         OnEntityChanged(index, NavGridTile::ENTITY_REMOVED, entity->GetObjectId(), entity);
      }
   });
}

void SensorTracker::RemoveAllEntitiesFrom(csg::Point3 const& index)
{
   ST_LOG(5) << "removing all entities at tile " << index;

   auto i = tile_map_.find(index);
   if (i != tile_map_.end()) {
      EntityFlatMap entity_map = std::move(i->second).entities;
      tile_map_.erase(i);

      for (const auto& entry : entity_map) {
         TryRemoveEntityFromSensor(entry.first);
      }
   }
}

void SensorTracker::OnEntityChanged(csg::Point3 const& index, int f, dm::ObjectId entityId, om::EntityPtr entity)
{
   om::SensorPtr sensor = sensor_.lock();
   if (sensor) {
      switch (f) {
      case NavGridTile::ENTITY_REMOVED:
         OnEntityRemovedFromTile(index, *sensor, entityId);
         break;

      case NavGridTile::ENTITY_ADDED:
      case NavGridTile::ENTITY_MOVED:
         OnEntityAddedToTile(index, *sensor, entityId, entity);
         break;
      }
   }
}

void SensorTracker::OnEntityRemovedFromTile(csg::Point3 const& index, om::Sensor& s, dm::ObjectId entityId)
{
   ST_LOG(5) << "checking status for entity " << entityId << " in OnEntityRemovedFromTile at " << index;

   auto i = tile_map_.find(index);
   if (i != tile_map_.end()) {
      EntityFlatMap& entity_map = i->second.entities;
      auto j = entity_map.find(entityId);
      if (j != entity_map.end()) {
         entity_map.erase(j);
         ST_LOG(7) << "was tracked by tile!  seeing if we need to remove it...";
         TryRemoveEntityFromSensor(entityId);
      }
   }
}

void SensorTracker::OnEntityAddedToTile(csg::Point3 const& index, om::Sensor& s, dm::ObjectId entityId, om::EntityRef e)
{
   ST_LOG(5) << "checking status for entity " << entityId << " in OnEntityAddedToTile at " << index;

   auto i = tile_map_.find(index);
   ASSERT(i != tile_map_.end());
   if (i != tile_map_.end()) {
      i->second.entities[entityId] = e;
      TryAddEntityToSensor(entityId, e);
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
         for (auto const& map : tile_map_) {
            if (stdutil::contains(map.second.entities, entityId)) {
               ST_LOG(7) << "entity " << entityId << " still in the tile_map.  not removing from sensor.";
               return;
            }
         }
         ST_LOG(7) << "entity " << entityId << " eradicated from the tile_map.  removing from sensor.";
         contents.Remove(entityId);
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
      } else {
         ST_LOG(7) << "entity " << *e.lock() << " already in the tile_map.  not adding to sensor.";
      }
   }
}
