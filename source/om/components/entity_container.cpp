#include "pch.h"
#include "entity_container.h"
#include "mob.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

void EntityContainer::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("children", children_);
}

void EntityContainer::AddChild(om::EntityRef c)
{
   auto child = c.lock();
   if (child) {
      auto mob = child->AddComponent<Mob>();
      auto parent = mob->GetParent();
      if (parent) {
         auto container = parent->GetEntity().GetComponent<EntityContainer>();
         container->RemoveChild(c);
      }
      mob->SetParent(GetEntity().GetComponent<Mob>());

      dm::ObjectId id = child->GetObjectId();
      children_[id] = child;
      destroy_guards_[id] = child->TraceObjectLifetime("remove from ec on destroy", [=]() {
         children_.Remove(id);
      });
   }
}

void EntityContainer::RemoveChild(om::EntityRef c)
{
   auto child = c.lock();
   if (child) {
      dm::ObjectId id = child->GetObjectId();
      children_.Remove(id);
      destroy_guards_.erase(id);
      auto mob = child->GetComponent<Mob>();
      if (mob) {
         mob->SetParent(nullptr);
      }
   }
}
