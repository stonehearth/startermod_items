#include "pch.h"
#include "carry_block.ridl.h"
#include "entity_container.ridl.h"
#include "dm/boxed_trace.h"
#include "mob.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, CarryBlock const& o)
{
   return (os << "[CarryBlock]");
}


void CarryBlock::ExtendObject(json::Node const& obj)
{
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
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         om::EntityPtr parent = mob->GetParent().lock();
         if (parent) {
            parent->GetComponent<EntityContainer>()->RemoveChild(entity->GetObjectId());
         }
         mob->SetParent(EntityRef());
         mob->SetLocationGridAligned(csg::Point3::zero);
      }
   }
   return *this;
}

void CarryBlock::ClearCarrying()
{
   carrying_ = EntityRef();
}
