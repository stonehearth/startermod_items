#include "pch.h"
#include "attached_items.ridl.h"
#include "entity_container.ridl.h"
#include "mob.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define AT_LOG(level)    LOG(om.attached_items, level)

std::ostream& operator<<(std::ostream& os, AttachedItems const& o)
{
   return (os << "[AttachedItems]");
}

void AttachedItems::LoadFromJson(json::Node const& obj)
{
}

void AttachedItems::SerializeToJson(json::Node& node) const
{
}

AttachedItems& AttachedItems::AddItem(std::string key, std::weak_ptr<Entity> value)
{ 
   om::EntityPtr item = value.lock();

   if (item) {
      if (items_.Contains(key)) {
         AT_LOG(1) << "Warning: replacing existing item on bone " << key << " without removing it first.  Make sure it is not orphaned.";
      }

      AT_LOG(3) << GetEntity() << " putting " << *item << " on bone " << key;
      items_.Add(key, value);

      om::MobPtr mob = item->GetComponent<om::Mob>();

      if (mob) {
         om::EntityPtr parent = mob->GetParent().lock();

         if (parent) {
            AT_LOG(5) << GetEntity() << " removing " << *item << " from parent " << *parent;
            parent->GetComponent<EntityContainer>()->RemoveChild(item->GetObjectId());
         }
         AT_LOG(5) << GetEntity() << " moving to zero";
         mob->SetLocationGridAligned(csg::Point3::zero);
      }
   } else {
      AT_LOG(3) << GetEntity() << " clearing bone " << key;
      items_.Remove(key);
   }

   return *this;
}

bool AttachedItems::ContainsItem(std::string const& key) const
{
   return items_.Contains(key);
}
