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

OctTree::OctTree(int trace_category) :
   trace_category_(trace_category),
   navgrid_(trace_category)
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
                                             ->OnAdded([this, id](std::string const& name, std::shared_ptr<dm::Object> obj) {
                                                OnComponentAdded(id, obj);
                                             })
                                             ->PushObjectState();
      }
   }
}

void OctTree::OnComponentAdded(dm::ObjectId id, std::shared_ptr<dm::Object> component)
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
            om::SensorListPtr sensor_list = std::static_pointer_cast<om::SensorList>(component);
            entry.sensor_list_trace = sensor_list->TraceSensors("octtree", trace_category_)
               ->OnAdded([this](std::string const& name, om::SensorPtr sensor) {
                  TraceSensor(sensor);
               })
               ->OnRemoved([this](std::string const& name) {
                  NOT_YET_IMPLEMENTED();
               })
               ->PushObjectState();
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
   //ASSERT(rootObject_);

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
      if (navgrid_.CanStandOn(entity, csg::ToClosestInt(point))) {
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

   // check both locations are standable
   if (!navgrid_.CanStandOn(entity, toLocation) || !navgrid_.CanStandOn(entity, fromLocation)) {
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
bool OctTree::ValidDiagonalMove(om::EntityPtr const& entity, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const
{
   std::vector<csg::Point3> candidates;
   csg::Point3 candidate, unused;

   // path 1 - x first, either y value (fromLocation.y or toLocation.y) ok
   candidate = fromLocation;
   candidate.x = toLocation.x;
   candidates.push_back(candidate);
   candidate.y = toLocation.y;
   candidates.push_back(candidate);

   if (!CanStandOnOneOf(entity, candidates, unused)) {
      return false;
   }

   // path 2 - z first, either y value (fromLocation.y or toLocation.y) ok
   candidate = fromLocation;
   candidate.z = toLocation.z;
   candidates.push_back(candidate);
   candidate.y = toLocation.y;
   candidates.push_back(candidate);

   if (!CanStandOnOneOf(entity, candidates, unused)) {
      return false;
   }

   return true;
}

std::vector<std::pair<csg::Point3, int>> OctTree::ComputeNeighborMovementCost(om::EntityPtr entity, const csg::Point3& from) const
{
   std::vector<std::pair<csg::Point3, int>> result;

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
         if (navgrid_.CanStandOn(entity, to)) {
            result.push_back(std::make_pair(to, EstimateMovementCost(from, to)));
            break;
         }
      }
   }

   for (const auto& direction : diagonal_directions) {
      for (int dy = 1; dy >= -2; dy--) {
         csg::Point3 to = from + direction + csg::Point3(0, dy, 0);
         if (!navgrid_.CanStandOn(entity, to)) {
            continue;
         }
         if (!ValidDiagonalMove(entity, from, to)) {
            continue;
         }
         result.push_back(std::make_pair(to, EstimateMovementCost(from, to)));
         break;
      }
   }

   for (const auto& direction : vertical_directions) {
      csg::Point3 to = from + direction;
      if (navgrid_.CanStandOn(entity, to)) {
         result.push_back(std::make_pair(to, EstimateMovementCost(from, to)));
      }
   }
   return result;
}

int OctTree::EstimateMovementCost(const csg::Point3& start, const csg::Point3& end) const
{
   static int COST_SCALE = 10;
   int cost = 0;

   // it's fairly expensive to climb.
   cost += COST_SCALE * std::max(end.y - start.y, 0) * 2;

   // falling is super cheap.
   cost += std::max(start.y - end.y, 0);

   // diagonals need to be more expensive than cardinal directions
   int xCost = abs(end.x - start.x);
   int zCost = abs(end.z - start.z);
   int diagCost = std::min(xCost, zCost);
   int horzCost = std::max(xCost, zCost) - diagCost;

   cost += (int)((horzCost + diagCost * 1.414213562) * COST_SCALE);

   return cost;
}

int OctTree::EstimateMovementCost(const csg::Point3& src, const csg::Region3& dst) const
{
   return !dst.IsEmpty() ? EstimateMovementCost(src, dst.GetClosestPoint(src)) : INT_MAX;
}

int OctTree::EstimateMovementCost(const csg::Point3& src, const std::vector<csg::Point3>& points) const
{
   int cost = INT_MAX;
   for (const auto& pt : points) {
      cost = std::min(cost, EstimateMovementCost(src, pt));
   }
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
   if (rootObject_ && id == rootObject_->GetObjectId()) {
      rootObject_ = nullptr;
   }
}
#endif

void OctTree::Update(int now)
{
   UpdateSensors();
}

bool OctTree::UpdateSensor(om::SensorPtr sensor)
{
   auto entity = sensor->GetEntity().lock();
   if (!entity) {
      return false;
   }
   auto mob = entity->GetComponent<om::Mob>();
   if (!mob) {
      return false;
   }
   csg::Point3f origin = mob->GetWorldLocation();
   csg::Cube3f cube = sensor->GetCube() + origin;

   std::unordered_map<dm::ObjectId, om::EntityRef> intersects;
   auto i = entities_.begin();
   while (i != entities_.end()) {
      om::EntityPtr entity = i->second.entity.lock();
      if (entity) {
         om::MobPtr mob = entity->GetComponent<om::Mob>();
         if (mob) {
            csg::Cube3f const& box = mob->GetWorldAabb();
            if (cube.Intersects(box)) {
               intersects[entity->GetObjectId()] = entity;
            }
         }
         i++;
      } else {
         i = entities_.erase(i);
      }
   }
   sensor->UpdateIntersection(intersects);
   return true;
}

void OctTree::TraceSensor(om::SensorPtr sensor)
{
   dm::ObjectId id = sensor->GetObjectId();
   sensors_[id] = sensor;
}


void OctTree::UpdateSensors()
{
   auto i = sensors_.begin();
   while (i != sensors_.end()) {
      om::SensorPtr sensor = i->second.lock();
      if (sensor && UpdateSensor(sensor)) {
         i++;
      } else {
         i = sensors_.erase(i);
      }
   }
}

void OctTree::ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg)
{
   navgrid_.ShowDebugShapes(pt, msg);
}

bool OctTree::CanStandOn(om::EntityPtr entity, const csg::Point3& at) const
{
   return navgrid_.CanStandOn(entity, at);
}

void OctTree::RemoveNonStandableRegion(om::EntityPtr e, csg::Region3& r) const
{
   return navgrid_.RemoveNonStandableRegion(e, r);
}
