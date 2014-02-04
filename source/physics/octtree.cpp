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
   static const csg::Point3 climb[] = {
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
         // Test all 3 points
         csg::Point3 to = from + csg::Point3(0, dy, 0);
         to.x += direction.x; // the point on the "left"
         if (!navgrid_.CanStandOn(entity, to)) {
            continue;
         }
         to.x -= direction.x;
         to.z += direction.z; // the point "front"
         if (!navgrid_.CanStandOn(entity, to)) {
            continue;
         }
         to.x += direction.x; // the point on the diagonal
         if (!navgrid_.CanStandOn(entity, to)) {
            continue;
         }
         // add the point on the diagonal
         result.push_back(std::make_pair(to, EstimateMovementCost(from, to)));
         break;
      }
   }

   for (const auto& direction : climb) {
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

   cost += (int)((horzCost + diagCost * 1.414) * COST_SCALE);

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
   // UpdateSensors();
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
