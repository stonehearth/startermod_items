#include "pch.h"
#include "entity_container.ridl.h"
#include "mob.ridl.h"
#include "om/entity.h"
#include "dm/trace.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, EntityContainer const& o)
{
   return (os << "[EntityContainer]");
}

void EntityContainer::ExtendObject(json::Node const& obj)
{
}

void EntityContainer::AddChild(std::weak_ptr<Entity> c)
{
   auto child = c.lock();
   if (child) {
      auto mob = child->AddComponent<Mob>();
      auto parent = mob->GetParent().lock();
      if (parent) {
         auto container = parent->GetEntity().GetComponent<EntityContainer>();
         container->RemoveChild(child->GetObjectId());
      }
      mob->SetParent(GetEntity().AddComponent<Mob>());

      dm::ObjectId id = child->GetObjectId();
      children_.Add(id, c);
      auto trace = child->TraceObjectChanges("ec dtor", dm::OBJECT_MODEL_TRACES);
      trace->OnDestroyed_([this, id]() {
         children_.Remove(id);
      });
      destroy_traces_[id] = trace;
   }
}

EntityContainer& EntityContainer::RemoveChild(dm::ObjectId id)
{
   auto i = children_.find(id);
   if (i != children_.end()) {
      EntityPtr child = i->second.lock();
      children_.Remove(id);
      destroy_traces_.erase(id);
      auto mob = child->GetComponent<Mob>();
      if (mob) {
         mob->SetParent(MobRef());
      }
   }
   return *this;
}
