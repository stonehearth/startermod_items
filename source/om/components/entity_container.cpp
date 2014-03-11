#include "pch.h"
#include "entity_container.ridl.h"
#include "mob.ridl.h"
#include "om/entity.h"
#include "dm/trace.h"
#include "dm/record_trace.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, EntityContainer const& o)
{
   return (os << "[EntityContainer]");
}

void EntityContainer::LoadFromJson(json::Node const& obj)
{
}

void EntityContainer::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   for (auto const& entry : EachChild()) {
      om::EntityPtr child = entry.second.lock();
      if (child) {
         node.set(stdutil::ToString(entry.first), child->GetStoreAddress());
      }
   }
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
      dtor_traces_[id] = child->TraceChanges("entity container lifetime", dm::OBJECT_MODEL_TRACES)
         ->OnDestroyed([this, id]() {
            children_.Remove(id);
         });
      children_.Add(id, c);
   }
}

EntityContainer& EntityContainer::RemoveChild(dm::ObjectId id)
{
   auto i = children_.find(id);
   if (i != children_.end()) {
      EntityPtr child = i->second.lock();
      children_.Remove(id);
      dtor_traces_.erase(id);
      auto mob = child->GetComponent<Mob>();
      if (mob) {
         mob->SetParent(EntityRef());
      }
   }
   return *this;
}
