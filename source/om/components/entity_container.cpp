#include "pch.h"
#include "entity_container.ridl.h"
#include "mob.ridl.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

EntityContainer& EntityContainer::InsertChild(dm::ObjectId key, std::weak_ptr<Entity> c)
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
      children_.Insert(id, c);
      auto trace = child->TraceObjectChanges("ec dtor", dm::OBJECT_MODEL_TRACES);
      trace->OnDestroyed([this, id]() {
         children_.Remove(id);
      });
      destroy_traces_[id] = trace;
   }
   return *this;
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
