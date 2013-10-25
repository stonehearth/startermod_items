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
   void ExtendObject(json::Node const& obj);

public:
   //std::string GetKind() const { return *kind_; }
   //void SetKind(std::string kind) { kind_ = kind; }

   dm::Boxed<float> const& GetScale() const { return scale_; }
   dm::Boxed<std::string> const& GetModelVariant() const { return model_variant_; }
   dm::Boxed<std::string> const& GetAnimationTable() const { return animation_table_; } 
   dm::Boxed<std::string> const& GetModelMode() const { return model_mode_; }
   dm::Boxed<std::string> const& GetMaterial() const { return material_; }
   dm::Set<om::EntityRef> const& GetAttachedEntities() const { return attached_; };

   void SetModelVariant(std::string const& value) { model_variant_ = value; }
   void SetAnimationTable(std::string const& value) { animation_table_ = value; }
   void SetModelMode(std::string const& value) { model_mode_ = value; }
   void SetMaterial(std::string const& value) { material_ = value; }
   
   void AttachEntity(om::EntityRef e);
   EntityRef RemoveEntity(std::string const& uri);

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
   dm::Boxed<std::string>        material_;
   dm::Boxed<std::string>        model_mode_;
   dm::Boxed<float>              scale_;

};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_RENDER_INFO_H
