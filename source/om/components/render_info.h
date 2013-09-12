#ifndef _RADIANT_OM_RENDER_INFO_H
#define _RADIANT_OM_RENDER_INFO_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class RenderInfo : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(RenderInfo, render_info);
   void ExtendObject(json::ConstJsonObject const& obj);

public:
   //std::string GetKind() const { return *kind_; }
   //void SetKind(std::string kind) { kind_ = kind; }

   dm::Boxed<std::string>& GetModelVariant() { return model_variant_; }
   dm::Boxed<std::string> const& GetModelVariant() const { return model_variant_; }

   std::string GetAnimationTable() const { return *animation_table_; } 
   float GetScale() const { return *scale_; }
   dm::Boxed<float> const& GetBoxedScale() const { return scale_; }

   dm::Set<om::EntityRef> const& GetAttachedEntities() const { return attached_; };
   void AttachEntity(om::EntityRef e);
   EntityRef RemoveEntity(std::string const& mod_name, std::string const& entity_name);

private:
   void RemoveFromWorld(EntityPtr entity);

private:
   NO_COPY_CONSTRUCTOR(RenderInfo)
   void InitializeRecordFields() override;

public:
   dm::Boxed<om::EntityRef>      attached_to_;
   dm::Boxed<std::string>        model_variant_;
   dm::Set<om::EntityRef>        attached_;
   dm::Boxed<std::string>        animation_table_;
   dm::Boxed<float>              scale_;

};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_RENDER_INFO_H
