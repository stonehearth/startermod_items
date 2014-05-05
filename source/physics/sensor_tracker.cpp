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

// Logs the contents of the sensor, if the physics.sensor_tracker log level exceeds the
// specified level. 

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
 * Create a trace on the Mob component of the Entity owning this sensor so we can
 * track its position in the world.
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
 * Called when the Entity the sensor is attached to moves.  If this results in a
 * change in our bounds, re-evaluate the status of all the SensorTileTrackers.
 */
void SensorTracker::OnSensorMoved()
{
   om::MobPtr mob = mob_.lock();
   om::SensorPtr sensor = sensor_.lock();
   if (sensor && mob) {
      csg::Point3 location = mob->GetWorldGridLocation();
      bounds_ = sensor->GetCube().Translated(location);

      ST_LOG(3) << "sensor moved to " << location << ".  bounds are now " << bounds_ << "(last bounds: " << last_bounds_ << ")";
      if (bounds_ != last_bounds_) {
         TraceNavGridTiles();
         last_bounds_ = bounds_;
      }
   }
}

/* 
 * -- SensorTracker::GetNavGrid
 *
 * Return the NavGrid object.
 */
NavGrid& SensorTracker::GetNavGrid() const
{
   return navgrid_;
}

/* 
 * -- SensorTracker::GetBounds
 *
 * Return a csg::Cube3 representing the visible region of the sensor, in world
 * coordinates.
 */
csg::Cube3 const& SensorTracker::GetBounds() const
{
   return bounds_;
}

/* 
 * -- SensorTracker::TraceNavGridTiles
 *
 * Applies differences from last_boudns_ to bounds_ to all the SensorTileTrackers.
 * Trackers which are no longer in the bounds are removed, and trackers which have
 * moves from the interior region to the fringe (or vice versa) will recompute
 * their overlapping Entities.
 */
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

   // The `inner` cube represents the set of NavGridTiles for which Entities are
   // definitely inside the sensor.  It is much more efficient to track these, since
   // we don't have to check their collision shape against the sensor shape when they
   // move.  Use the `inner` box to determine which flags to pass to the SensorTileTracker
   // (ADDED only for things in `inner` and ADDED | CHANGED for things outside of
   // `inner`, otherwise knows as the "fringe").

   csg::Cube3 inner = csg::Cube3::Construct(current.min + csg::Point3(1, 1, 1),
                                            current.max - csg::Point3(1, 1, 1));

   for (csg::Point3 const& cursor : current) {
      int flags = NavGridTile::ENTITY_ADDED | NavGridTile::ENTITY_REMOVED;
      if (!inner.Contains(cursor)) {
         // We're on the fringe.  Need to subscribe to move events.
         flags |= NavGridTile::ENTITY_MOVED;
      }

      auto i = _sensorTileTrackers.find(cursor);
      if (i != _sensorTileTrackers.end()) {
         i->second->UpdateFlags(flags);
      } else {
         _sensorTileTrackers[cursor] = std::make_shared<SensorTileTracker>(*this, cursor, flags);
      }
   }
}


/* 
 * -- SensorTracker::OnEntityRemovedFromSensorTileTracker
 *
 * Called by a SensorTileTracker whenever an Entity leaves the tracker.  We keep a reference
 * count of how many SensorTileTrackers can see the entity, and only get rid of it when
 * that count goes to 0.
 */
void SensorTracker::OnEntityRemovedFromSensorTileTracker(dm::ObjectId entityId)
{
   auto sensor = sensor_.lock();
   if (sensor) {
      auto i = _entitiesTracked.find(entityId);
      ASSERT(i != _entitiesTracked.end());
      if (i != _entitiesTracked.end() && i->second == 1) {
         _entitiesTracked.erase(i);

         auto& contents = sensor->GetContainer();
         ST_LOG(7) << "reference count for entity " << entityId << " is now 0 in remove";
         ST_LOG(7) << "entity " << entityId << " eradicated from the tile_map.  removing from sensor.";
         contents.Remove(entityId);
         ST_LOG_SENSOR_CONTENTS(9, contents)
      } else {
         --i->second;
         ST_LOG(7) << "reference count for entity " << entityId << " is now " << i->second << " in remove";
      }
   }
}

/* 
 * -- SensorTracker::OnEntityAddedToSensorTileTracker
 *
 * Called by a SensorTileTracker whenever an Entity enters the tracker.  We keep a reference
 * count of how many SensorTileTrackers can see the entity, and only get rid of it when
 * that count goes to 0.
 */
void SensorTracker::OnEntityAddedToSensorTileTracker(dm::ObjectId entityId, om::EntityRef e)
{
   auto sensor = sensor_.lock();
   if (sensor) {
      auto i = _entitiesTracked.find(entityId);
      if (i != _entitiesTracked.end()) {
         ++i->second;
      } else {
         _entitiesTracked[entityId] = 1;

         auto& contents = sensor->GetContainer();
         ST_LOG(7) << "entity " << *e.lock() << " first added to tile map.  adding to sensor!.";
         contents.Add(entityId, e);
         ST_LOG_SENSOR_CONTENTS(9, contents)
      }
      ST_LOG(7) << "reference count for entity " << entityId << " is now " << _entitiesTracked[entityId] << " in add";
   }
}

/* 
 * -- SensorTracker::GetLogPrefix
 *
 * Helper for SensorTrackerTiles to make the log look prettier.
 */
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
