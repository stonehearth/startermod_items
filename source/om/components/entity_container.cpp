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
   AddChildToContainer(children_, c, "");
}

void EntityContainer::AddChildToBone(std::weak_ptr<Entity> c, std::string const& bone)
{
   AddChildToContainer(attached_items_, c, bone);
}

void EntityContainer::AddChildToContainer(dm::Map<dm::ObjectId, EntityRef> &children, EntityRef c, std::string const& bone)
{
   auto child = c.lock();
   if (child) {
      dm::ObjectId childId = child->GetObjectId();
      MobPtr mob = child->AddComponent<Mob>();
      EntityPtr parent = mob->GetParent().lock();
      if (parent) {
         auto container = parent->GetComponent<EntityContainer>();
         container->RemoveChild(childId);
      }
      if (mob->GetBone() != bone) {
         mob->SetBone(bone);
      }
      mob->SetParent(GetEntityRef());

      AddTrace(c);
      children.Add(childId, c);
   }
}

EntityContainer& EntityContainer::RemoveChild(dm::ObjectId id)
{
   EntityPtr child;
   auto i = children_.find(id);
   if (i != children_.end()) {
      children_.Remove(id);
      child = i->second.lock();
   } else {
      auto i = attached_items_.find(id);
      if (i != attached_items_.end()) {
         attached_items_.Remove(id);
         child = i->second.lock();
      }
   }
   if (child) {
      RemoveTrace(id);
      auto mob = child->GetComponent<Mob>();
      if (!mob->GetBone().empty()) {
         mob->SetBone("");
      }
      mob->SetParent(EntityRef());
   }
   return *this;
}

void EntityContainer::AddTrace(std::weak_ptr<Entity> c)
{
   auto child = c.lock();
   if (child) {
      dm::ObjectId childId = child->GetObjectId();
      dtor_traces_[childId] = child->TraceChanges("entity container lifetime", dm::OBJECT_MODEL_TRACES)
         ->OnDestroyed([this, childId]() {
            children_.Remove(childId);
         });
   }
}

void EntityContainer::RemoveTrace(dm::ObjectId childId)
{
   dtor_traces_.erase(childId);
}

void EntityContainer::OnLoadObject(dm::SerializationType r)
{
   if (r == dm::PERSISTANCE) {
      for (auto const& entry : children_) {
         AddTrace(entry.second);
      }
   }
}
