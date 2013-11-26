#include "pch.h"
#include "mob.ridl.h"
#include "render_info.ridl.h"
#include "entity_container.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, RenderInfo const& o)
{
   return (os << "[RenderInfo]");
}

void RenderInfo::ConstructObject()
{
   scale_ = 0.1f;
}

void RenderInfo::ExtendObject(json::Node const& obj)
{
   scale_ = obj.get<float>("scale", *scale_);
   material_ = obj.get<std::string>("material", *material_);
   model_mode_ = obj.get<std::string>("model_mode", *model_mode_);
   model_variant_ = obj.get<std::string>("model_variant", *model_variant_);
   animation_table_ = obj.get<std::string>("animation_table", *animation_table_);
}

void RenderInfo::AttachEntity(om::EntityRef e)
{
   auto entity = e.lock();
   if (entity) {
      RemoveFromWorld(entity);
      attached_entities_.Insert(e);
   }
}

EntityRef RenderInfo::RemoveEntity(std::string const& uri)
{
   for (auto const& e : attached_entities_) {
      auto entity = e.lock();
      if (entity && entity->GetUri() == uri) {
         attached_entities_.Remove(e);
         return e;
      }
   }
   return EntityRef();
}

void RenderInfo::RemoveFromWorld(EntityPtr entity)
{
   auto mob = entity->GetComponent<om::Mob>();
   if (mob) {
      auto parent = mob->GetParent().lock();
      if (parent) {
         parent->GetEntity().GetComponent<EntityContainer>()->RemoveChild(entity->GetObjectId());
      }
      ASSERT(mob->GetParent().expired());
   }
}
