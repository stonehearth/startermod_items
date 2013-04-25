#ifndef _RADIANT_OM_COMPONENT_H
#define _RADIANT_OM_COMPONENT_H

#include "om/om.h"
#include "om/entity.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "om/all_object_types.h"

BEGIN_RADIANT_OM_NAMESPACE
   
class Component : public dm::Record
{
public:
   Component() { }

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

   EntityPtr GetEntityPtr() const { return (*entity_).lock(); }
   EntityRef GetEntityRef() const { return (*entity_); }
   Entity& GetEntity() const { return *(*entity_).lock(); }
   void SetEntity(EntityPtr entity) { entity_ = entity; }

   const math3d::ipoint3 GetOrigin() const;
   const math3d::ipoint3 GetWorldOrigin() const;

protected:
   void InitializeRecordFields() {
      AddRecordField("entity", entity_);
   }

private:
   dm::Boxed<EntityRef>    entity_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_COMPONENT_H
