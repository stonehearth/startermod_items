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
      MobPtr mob = child->AddComponent<Mob>();
      EntityPtr parent = mob->GetParent().lock();
      if (parent) {
         auto container = parent->GetComponent<EntityContainer>();
         container->RemoveChild(child->GetObjectId());
      }
      mob->SetParent(GetEntityRef());

      dm::ObjectId id = child->GetObjectId();
      children_.Add(id, c);
   }
}

EntityContainer& EntityContainer::RemoveChild(dm::ObjectId id)
{
   auto i = children_.find(id);
   if (i != children_.end()) {
      EntityPtr child = i->second.lock();
      children_.Remove(id);
      auto mob = child->GetComponent<Mob>();
      if (mob) {
         mob->SetParent(EntityRef());
      }
   }
   return *this;
}
