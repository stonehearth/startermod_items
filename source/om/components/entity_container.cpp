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
      children_[child->GetObjectId()] = child;
   }
}

void EntityContainer::RemoveChild(om::EntityRef c)
{
   auto child = c.lock();
   if (child) {
      children_.Remove(child->GetEntityId());
      auto mob = child->GetComponent<Mob>();
      if (mob) {
         mob->SetParent(nullptr);
      }
   }
}
