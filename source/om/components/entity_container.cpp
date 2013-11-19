#include "pch.h"
#include "entity_container.h"
#include "mob.h"
#include "om/entity.h"
#include "om/trace_categoies.h"

using namespace ::radiant;
using namespace ::radiant::om;

void EntityContainer::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("children", children_);
}

EntityContainer& EntityContainer::AddChild(om::EntityRef c)
{
   auto child = c.lock();
   if (child) {
      auto mob = child->AddComponent<Mob>();
      auto parent = mob->GetParent();
      if (parent) {
         auto container = parent->GetEntity().GetComponent<EntityContainer>();
         container->RemoveChild(c);
      }
      mob->SetParent(GetEntity().AddComponent<Mob>());

      dm::ObjectId id = child->GetObjectId();
      children_.Insert(id, child);
      auto trace = child->TraceObjectChanges("ec dtor", OM_TRACES);
      trace->OnDestroyed([this, id]() {
         children_.Remove(id);
      });
      destroy_traces_[id] = trace;
   }
   return *this;
}

EntityContainer& EntityContainer::RemoveChild(om::EntityRef c)
{
   auto child = c.lock();
   if (child) {
      dm::ObjectId id = child->GetObjectId();
      children_.Remove(id);
      destroy_traces_.erase(id);
      auto mob = child->GetComponent<Mob>();
      if (mob) {
         mob->SetParent(nullptr);
      }
   }
   return *this;
}
