#include "radiant.h"
#include "radiant_stdutil.h"
#include "octtree.h"
#include "metrics.h"
#include "csg/ray.h"
#include "csg/region.h"
#include "csg/util.h" // xxx: should be csg/namespace.h
#include "om/entity.h"
#include "dm/map_trace.h"
#include "om/components/mob.ridl.h"
#include "om/components/entity_container.ridl.h"
#include "om/components/region_collision_shape.ridl.h"
#include "om/components/sensor.ridl.h"
#include "om/components/sensor_list.ridl.h"
#include "sensor_tracker.h"

using namespace radiant;
using namespace radiant::phys;

#define OT_LOG(level)   LOG(physics.collision, level)

static const csg::Point3 _adjacent[] = {
   csg::Point3( 1,  0,  0),
   csg::Point3(-1,  0,  0),
   csg::Point3( 0,  0, -1),
   csg::Point3( 0,  0,  1),

   //csg::Point3( 1, -1,  0),
   //csg::Point3(-1, -1,  0),
   //csg::Point3( 0, -1, -1),
   //csg::Point3( 0, -1,  1),

   //csg::Point3( 1,  1,  0),
   //csg::Point3(-1,  1,  0),
   //csg::Point3( 0,  1, -1),
   //csg::Point3( 0,  1,  1),
};

template <class T>
T WorldToLocal(const T& coord, const om::Entity& entity)
{
   csg::Point3f origin(0, 0, 0);
   auto mob = entity.GetComponent<om::Mob>();
   if (mob) {
      origin = mob->GetWorldLocation();
   }
   return coord - origin;
}

OctTree::OctTree(dm::TraceCategories trace_category) :
   trace_category_(trace_category),
   navgrid_(trace_category),
   enable_sensor_traces_(false)
{
}

void OctTree::Cleanup()
{
}

void OctTree::SetRootEntity(om::EntityPtr root)
{
   TraceEntity(root);
}

void OctTree::TraceEntity(om::EntityPtr entity)
{
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      if (!stdutil::contains(entities_, id)) {
         EntityMapEntry& entry = entities_[id];
         entry.entity = entity;
         entry.components_trace = entity->TraceComponents("octtree", trace_category_)
                                             ->OnAdded([this, id](std::string const& name, om::ComponentPtr component) {
                                                OnComponentAdded(id, component);
                                             })
                                             ->OnDestroyed([this, id]() {
                                                entities_.erase(id);
                                             })
                                             ->PushObjectState();
      }
   }
}

void OctTree::OnComponentAdded(dm::ObjectId id, om::ComponentPtr component)
{
   auto i = entities_.find(id);
   if (i == entities_.end()) {
      return;
   }
   EntityMapEntry& entry = i->second;
   om::EntityPtr entity = entry.entity.lock();
   
   if (entity) {
      switch (component->GetObjectType()) {
         case om::EntityContainerObjectType: {
            om::EntityContainerPtr entity_container = std::static_pointer_cast<om::EntityContainer>(component);
            entry.children_trace = entity_container->TraceChildren("octtree", trace_category_)
               ->OnAdded([this](dm::ObjectId id, om::EntityRef e) {
                     TraceEntity(e.lock());
               })
               ->PushObjectState();
            break;
         };
         case om::SensorListObjectType: {
            if (enable_sensor_traces_) {
               om::EntityRef e = entity;
               om::SensorListPtr sensor_list = std::static_pointer_cast<om::SensorList>(component);
               entry.sensor_list_trace = sensor_list->TraceSensors("oct tree", trace_category_)
                  ->OnAdded([this, e](std::string const& name, om::SensorPtr sensor) {
                     dm::ObjectId id = sensor->GetObjectId();

                     dm::TracePtr dtorTrace = sensor->TraceObjectChanges("octtree dtor", trace_category_);
                     dtorTrace->OnDestroyed_([this, id] {
                        sensor_trackers_.erase(id);
                     });

                     ASSERT(!stdutil::contains(sensor_trackers_, id));
                     SensorTrackerPtr sensorTracker = std::make_shared<SensorTracker>(navgrid_, e.lock(), sensor);
                     sensorTracker->Initialize();

                     sensor_trackers_[id] = std::make_pair(sensorTracker, dtorTrace);
                  })
                  ->OnRemoved([this](std::string const& name) {
                     NOT_YET_IMPLEMENTED();
                  })
                  ->PushObjectState();
            }
         }
      }
      navgrid_.TrackComponent(component);
   }       
}

#if 0
void OctTree::GetActorsIn(const csg::Cube3f &bounds, QueryCallback cb)
{
   auto filter = [bounds, cb](om::EntityPtr entity) -> bool {
      auto a = entity->GetComponent<om::Mob>();
      auto shape = a->GetWorldAABB();
      OT_LOG(5) << a->GetType() << " " << a->GetId() << " bounds:" << bounds << " shape:" << shape << " hit:" << bounds.Intersects(shape);
      if (shape.Intersects(bounds)) {
         cb(entity);
         return true;
      }
      return false;
   };
   FilterAllActors(filter);
}
#endif

void OctTree::TraceRay(const csg::Ray3 &ray, RayQueryCallback cb)
{
   //ASSERT(rootEntity_);

   auto filter = [ray, cb](om::EntityPtr entity) -> bool {
      float t;
      auto mob = entity->GetComponent<om::Mob>();
      if (mob) {
         csg::Cube3f box = mob->GetWorldAabb();
         if (csg::Cube3Intersects(box, ray, t)) {
            cb(entity, t);
            return true;
         }
      }
      return false;
   };
   FilterAllActors(filter);
}

void OctTree::FilterAllActors(std::function <bool(om::EntityPtr)> filter)
{
   auto i = entities_.begin();
   while (i != entities_.end()) {
      om::EntityPtr entity = i->second.entity.lock();
      if (entity) {
         filter(entity);
         i++;
      } else {
         i = entities_.erase(i);
      }
   }
}

om::EntityPtr OctTree::FindFirstActor(om::EntityPtr root, std::function <bool(om::EntityPtr)> filter)
{
   if (filter(root)) {
      return root;
   }
   om::EntityContainerPtr container = root->GetComponent<om::EntityContainer>();
   if (container) {
      for (const auto& child : container->EachChild()) {
         auto entity = child.second.lock();
         if (entity) {
            auto result = FindFirstActor(entity, filter);
            if (result) {
               return result;
            }
         }
      }
   }
   return NULL;
}

template <class T>
bool OctTree::CanStandOnOneOf(om::EntityPtr const& entity, std::vector<csg::Point<T,3>> const& points, csg::Point<T,3>& standablePoint) const
{
   for (csg::Point<T,3> const& point : points) {
      if (navgrid_.IsStandable(entity, csg::ToClosestInt(point))) {
         standablePoint = point;
         return true;
      }
   }
   return false;
}

template bool OctTree::CanStandOnOneOf(om::EntityPtr const&, std::vector<csg::Point3> const& points, csg::Point3&) const;
template bool OctTree::CanStandOnOneOf(om::EntityPtr const&, std::vector<csg::Point3f> const& points, csg::Point3f&) const;

bool OctTree::ValidMove(om::EntityPtr const& entity, bool const reversible,
                        csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const
{
   int const dx = toLocation.x - fromLocation.x;
   int const dy = toLocation.y - fromLocation.y;
   int const dz = toLocation.z - fromLocation.z;

   // check if moving to one of the 8 adjacent x,z cells (or staying in the same cell)
   if (dx*dx > 1 || dz*dz > 1) {
      return false;
   }

   // check that destination is standable
   // not checking that source location is standable so that entities can move off of invalid squares (newly invalid, bugged, etc)
   if (!navgrid_.IsStandable(entity, toLocation)) {
      return false;
   }

   // check elevation changes
   if (dy != 0) {
      if (!ValidElevationChange(entity, reversible, fromLocation, toLocation)) {
         return false;
      }
   }

   // if diagonal, check diagonal constraints
   if (dx != 0 && dz != 0) {
      if (!ValidDiagonalMove(entity, fromLocation, toLocation)) {
         return false;
      }
   }

   return true;
}

bool OctTree::ValidElevationChange(om::EntityPtr const& entity, bool const reversible, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const
{
   // these constants should probably be exposed by the class
   static int const maxClimbUp = 1;
   static int const maxClimbDown = -maxClimbUp;
   static int const maxDrop = -2;

   int const dy = toLocation.y - fromLocation.y;

   if (dy > maxClimbUp) {
      return false;
   }

   int const dx = toLocation.x - fromLocation.x;
   int const dz = toLocation.z - fromLocation.z;

   // if climbing (no horizontal movement) or if reversible move
   if ((dx == 0 && dz == 0) || reversible) {
      if (dy < maxClimbDown) {
         return false;
      }
   } else {
      // drops must be adjacent and are non-reversible
      if (dy < maxDrop) {
         return false;
      }
   }

   return true;
}

// tests diagonal specific requirements
// the two adjacent non-diagonal paths to the destination must be walkable with a height equal to either the from or to location
bool OctTree::ValidDiagonalMove(om::EntityPtr const& entity, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const
{
   std::vector<csg::Point3> candidates;
   csg::Point3 candidate, unused;

   // path 1 - x first, y value of fromLocation
   candidate = fromLocation;
   candidate.x = toLocation.x;
   candidates.push_back(candidate);

   if (candidate.y != toLocation.y) {
      // path 1 - x first, y value of toLocation
      candidate.y = toLocation.y;
      candidates.push_back(candidate);
   }

   if (!CanStandOnOneOf(entity, candidates, unused)) {
      return false;
   }

   candidates.clear();

   // path 2 - z first, y value of fromLocation
   candidate = fromLocation;
   candidate.z = toLocation.z;
   candidates.push_back(candidate);

   if (candidate.y != toLocation.y) {
      // path 2 - z first, y value of toLocation
      candidate.y = toLocation.y;
      candidates.push_back(candidate);
   }

   if (!CanStandOnOneOf(entity, candidates, unused)) {
      return false;
   }

   return true;
}

OctTree::MovementCostVector OctTree::ComputeNeighborMovementCost(om::EntityPtr entity, const csg::Point3& from) const
{
   MovementCostVector result;

   // xxx: this is in no way thread safe! (see SH-8)
   static const csg::Point3 cardinal_directions[] = {
      // cardinals and diagonals
      csg::Point3( 1, 0, 0 ),
      csg::Point3(-1, 0, 0 ),
      csg::Point3( 0, 0, 1 ),
      csg::Point3( 0, 0,-1 ),
   };
   static const csg::Point3 diagonal_directions[] = {
      csg::Point3( 1, 0, 1 ),
      csg::Point3(-1, 0,-1 ),
      csg::Point3(-1, 0, 1 ),
      csg::Point3( 1, 0,-1 ),
   };
   // xxx: this is in no way thread safe! (see SH-8)
   static const csg::Point3 vertical_directions[] = {
      // ladder climbing
      csg::Point3( 0, 1, 0 ),
      csg::Point3( 0,-1, 0 ),
   };

   for (const auto& direction : cardinal_directions) {
      for (int dy = 1; dy >= -2; dy--) {
         csg::Point3 to = from + direction + csg::Point3(0, dy, 0);
         if (navgrid_.IsStandable(entity, to)) {
            result.push_back(std::make_pair(to, GetMovementCost(from, to)));
            break;
         }
      }
   }

   for (const auto& direction : diagonal_directions) {
      for (int dy = 1; dy >= -2; dy--) {
         csg::Point3 to = from + direction + csg::Point3(0, dy, 0);
         if (!navgrid_.IsStandable(entity, to)) {
            continue;
         }
         if (!ValidDiagonalMove(entity, from, to)) {
            continue;
         }
         result.push_back(std::make_pair(to, GetMovementCost(from, to)));
         break;
      }
   }

   for (const auto& direction : vertical_directions) {
      csg::Point3 to = from + direction;
      if (navgrid_.IsStandable(entity, to)) {
         result.push_back(std::make_pair(to, GetMovementCost(from, to)));
      }
   }
   return result;
}


/*
 * -- OctTree::GetMovementCost
 *
 * Get the actual cost to move from the start to the finish.  It's up to the caller
 * to validate the parameters.
 */

float OctTree::GetMovementCost(const csg::Point3& start, const csg::Point3& end) const
{
   float cost = 0;

   // it's fairly expensive to climb (xxx: except climbing a ladder should be free!)
   cost += std::max(end.y - start.y, 0) * 2;

   // falling is super cheap (free, in fact, but leave this here just in case).
   cost += std::max(start.y - end.y, 0);

   int dx = abs(end.x - start.x);
   int dz = abs(end.z - start.z);
   cost += static_cast<float>(std::sqrt(dx*dx + dz*dz));

   return cost;
}

#if 0
void OctTree::OnObjectCreated(om::EntityPtr entity)
{
   allActors_.push_back(entity);

   auto shape = entity->GetComponent<om::GridCollisionShape>();
   if (shape) {
      collisionShapes_.push_back(entity);
   }
   auto pathJump = entity->GetComponent<om::PathJumpNode>();
   if (pathJump) {
      pathJumpNodes_.push_back(pathJump);
   }
}

void OctTree::OnObjectDestroyed(dm::ObjectId id)
{
   if (rootEntity_ && id == rootEntity_->GetObjectId()) {
      rootEntity_ = nullptr;
   }
}
#endif

void OctTree::ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg)
{
   navgrid_.ShowDebugShapes(pt, msg);
}


void OctTree::EnableSensorTraces(bool enabled)
{
   enable_sensor_traces_ = enabled;
}
