#include "pch.h"
#include "render_info.h"
#include "mob.h"
#include "entity_container.h"

using namespace ::radiant;
using namespace ::radiant::om;

void RenderInfo::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("render_mode", model_variant_);
   AddRecordField("animation_table", animation_table_);
   AddRecordField("scale", scale_);
   AddRecordField("attached", attached_);
   AddRecordField("material", material_);
   if (!IsRemoteRecord()) {
      scale_ = 0.1f;
   }
}


void RenderInfo::ExtendObject(json::Node const& obj)
{
   scale_ = obj.get<float>("scale", *scale_);
   material_ = obj.get<std::string>("material", *material_);
   model_variant_ = obj.get<std::string>("model_variant", *model_variant_);
   animation_table_ = obj.get<std::string>("animation_table", *animation_table_);
}

void RenderInfo::AttachEntity(om::EntityRef e)
{
   auto entity = e.lock();
   if (entity) {
      RemoveFromWorld(entity);
      attached_.Insert(e);
   }
}

EntityRef RenderInfo::RemoveEntity(std::string const& uri)
{
   for (auto const& e : attached_) {
      auto entity = e.lock();
      if (entity && entity->GetUri() == uri) {
         attached_.Remove(e);
         return e;
      }
   }
   return EntityRef();
}

void RenderInfo::RemoveFromWorld(EntityPtr entity)
{
   auto mob = entity->GetComponent<om::Mob>();
   if (mob) {
      auto parent = mob->GetParent();
      if (parent) {
         parent->GetEntity().GetComponent<EntityContainer>()->RemoveChild(entity);
      }
      ASSERT(mob->GetParent() == nullptr);
   }
}
