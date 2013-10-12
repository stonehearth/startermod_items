#include "radiant.h"
#include "octtree.h"
#include "metrics.h"
#include "om/entity.h"
#include "om/components/mob.h"
#include "om/components/entity_container.h"
#include "om/components/collision_shape.h"
#include "om/components/sphere_collision_shape.h"
#include "om/components/sensor_list.h"

using namespace radiant;
using namespace radiant::phys;

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

OctTree::OctTree()
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
         entities_[id] = entity;

         const auto& components = entity->GetComponents();

         guards_ += components.TraceMapChanges("octtree component checking",
                                               std::bind(&OctTree::OnComponentAdded, this, om::EntityRef(entity), std::placeholders::_1, std::placeholders::_2),
                                               nullptr);

         for (const auto& entry : components) {
            OnComponentAdded(entity, entry.first, entry.second);
         }
      }
   }
}

void OctTree::OnComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component)
{
   om::EntityPtr entity = e.lock();

   if (entity) {
      switch (component->GetObjectType()) {
      case om::EntityContainerObjectType: {
         UpdateEntityContainer(std::static_pointer_cast<om::EntityContainer>(component));
         break;
      };
      case om::SensorListObjectType: {
         const auto& sensors = std::static_pointer_cast<om::SensorList>(component)->GetSensors();
         guards_ += sensors.TraceMapChanges("oct tree sensors",
                                            std::bind(&OctTree::TraceSensor, this, std::placeholders::_2),
                                            nullptr);
         for (const auto& entry : sensors) {
            TraceSensor(entry.second);
         }
         break;
      }
      }
      navgrid_.TrackComponent(type, component);
   }       
}

#if 0
void OctTree::GetActorsIn(const csg::Cube3f &bounds, QueryCallback cb)
{
   auto filter = [bounds, cb](om::EntityPtr entity) -> bool {
      auto a = entity->GetComponent<om::Mob>();
      auto shape = a->GetWorldAABB();
      // LOG(WARNING) << a->GetType() << " " << a->GetId() << " bounds:" << bounds << " shape:" << shape << " hit:" << bounds.Intersects(shape);
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
         csg::Cube3f box = mob->GetWorldAABB();
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
      om::EntityPtr entity = i->second.lock();
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
      for (const auto& child : container->GetChildren()) {
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

bool OctTree::IsStuck(om::EntityPtr entity)
{
   auto mob = entity->GetComponent<om::Mob>();
   return mob ? !navgrid_.IsEmpty(csg::ToInt(mob->GetLocation())) : false;
}

void OctTree::Unstick(std::vector<csg::Point3> &points)
{
   for (unsigned int i = 0; i < points.size(); i++) {
      points[i] = Unstick(points[i]);
   }
}

csg::Point3 OctTree::Unstick(const csg::Point3 &pt)
{
   csg::Point3 end = pt;

   while (!navgrid_.IsEmpty(end)) {
      end.y += 1;
   }
#if 0
   while (IsPassable(end)) {
      end.y -= 1;
   }
#endif

   end.y += 1;
   return csg::Point3(end);
}

void OctTree::Unstick(om::EntityPtr entity)
{
   auto mob = entity->GetComponent<om::Mob>();
   if (mob) {
      csg::Point3 end = csg::ToInt(mob->GetLocation());

      while (!navgrid_.IsEmpty(end)) {
         end.y += 1;
      }
      while (navgrid_.IsEmpty(end)) {
         end.y -= 1;
      }
      end.y += 1;
      mob->MoveTo(csg::ToFloat(end));
   }
}

std::vector<std::pair<csg::Point3, int>> OctTree::ComputeNeighborMovementCost(const csg::Point3& from) const
{
   std::vector<std::pair<csg::Point3, int>> result;

   // xxx: this is in no way thread safe! (see SH-8)
   static const csg::Point3 directions[] = {
      // cardinals and diagonals
      csg::Point3( 1, 0, 0 ),
      csg::Point3(-1, 0, 0 ),
      csg::Point3( 0, 0, 1 ),
      csg::Point3( 0, 0,-1 ),
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

   for (const auto& direction : directions) {
      csg::Point3 to;
      for (int dy = 1; dy >= -2; dy--) {
         csg::Point3 to = from + direction + csg::Point3(0, dy, 0);
         if (navgrid_.CanStand(to)) {
            result.push_back(std::make_pair(to, EstimateMovementCost(from, to)));
            break;
         }
      }
   }
   for (const auto& direction : climb) {
      csg::Point3 to = from + direction;
      if (navgrid_.CanStand(to)) {
         result.push_back(std::make_pair(to, EstimateMovementCost(from, to)));
      }
   }

#if 0
   for (const auto& v : verticals_) {
      auto vpr = v.lock();
      if (vpr) {
         auto region = vpr->GetRegionPtr();
         if (region) {
            const csg::Point3 f = csg::Point3(WorldToLocal(csg::Point3f(from), vpr->GetEntity()));
            const csg::Region3& rgn = **region;
            for (const auto& direction : climb) {
               csg::Point3 to = f + direction;
               if (rgn.Contains(f) && rgn.Contains(to) && IsPassable(to)) {
                  result.push_back(std::make_pair(from + direction, EstimateMovementCost(f, to)));
               }
            }
         }
      }
   }
#endif
   return result;
}

#if 0
bool OctTree::CanStepUp(csg::Point3& at) const
{
   for (int i = 0; i < 2; i++) {
      if (IsPassable(at)) {
         return true;
      }
      at.y += 1;
   }
   return false;
}

bool OctTree::CanFallDown(csg::Point3& at) const
{
   for (int i = 0; i < 2; i++) {
      if (CanStand(at)) {
         return true;
      }
      at.y -= 1;
   }
   return false;
}
#endif

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

   std::vector<dm::ObjectId> intersects;
   auto i = entities_.begin();
   while (i != entities_.end()) {
      auto entity = i->second.lock();
      if (entity) {
         auto mob = entity->GetComponent<om::Mob>();
         if (mob) {
            auto box = mob->GetWorldAABB();
            if (cube.Intersects(box)) {
               intersects.push_back(entity->GetObjectId());
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

void OctTree::UpdateEntityContainer(om::EntityContainerPtr container)
{
   const auto& children = container->GetChildren();

   auto on_change = [=](const dm::ObjectId, const om::EntityRef e) {
      TraceEntity(e.lock());
   };
   guards_ += children.TraceMapChanges("octtree entity children checking", on_change, nullptr);

   for (const auto& entry : children) {
      TraceEntity(entry.second.lock());
   }
}

TerrainChangeCbId OctTree::AddCollisionRegionChangeCb(csg::Region3 const* r, TerrainChangeCb cb)
{
   return navgrid_.AddCollisionRegionChangeCb(r, cb);
}

void OctTree::RemoveCollisionRegionChangeCb(TerrainChangeCbId id)
{
   navgrid_.RemoveCollisionRegionChangeCb(id);
}

