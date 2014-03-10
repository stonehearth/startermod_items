#include "pch.h"
#include "carry_block.ridl.h"
#include "entity_container.ridl.h"
#include "dm/boxed_trace.h"
#include "mob.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define CB_LOG(level)    LOG(om.carry_block, level)

std::ostream& operator<<(std::ostream& os, CarryBlock const& o)
{
   return (os << "[CarryBlock]");
}


void CarryBlock::LoadFromJson(json::Node const& obj)
{
}

void CarryBlock::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   om::EntityPtr carrying = GetCarrying().lock();
   if (carrying) {
      node.set("carrying", carrying->GetStoreAddress());
   } else {
      node.set("carrying", "");
   }   
}

bool CarryBlock::IsCarrying() const
{
   return GetCarrying().lock() != nullptr;
}

CarryBlock& CarryBlock::SetCarrying(std::weak_ptr<Entity> value)
{ 
   carrying_ = value;
   om::EntityPtr entity = value.lock();
   if (entity) {
      CB_LOG(3) << GetEntity() << " put " << *entity << " on carry bone";
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         om::EntityPtr parent = mob->GetParent().lock();
         if (parent) {
            CB_LOG(5) << GetEntity() << " removing " << *entity << " from parent " << *parent;
            parent->GetComponent<EntityContainer>()->RemoveChild(entity->GetObjectId());
         }
         CB_LOG(5) << GetEntity() << " moving to zero";
         mob->SetLocationGridAligned(csg::Point3::zero);
      }
   } else {
      CB_LOG(3) << GetEntity() << " cleared carry bone";
   }
   return *this;
}

void CarryBlock::ClearCarrying()
{
   carrying_ = EntityRef();
}
