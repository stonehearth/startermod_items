#ifndef _RADIANT_OM_COMPONENT_H
#define _RADIANT_OM_COMPONENT_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/set.h"
#include "om/entity.h"
#include "lib/json/node.h"

BEGIN_RADIANT_OM_NAMESPACE
   
class Component : public dm::Record
{
public:
   Component() { }

   virtual void ExtendObject(json::Node const& obj) { };

   EntityPtr GetEntityPtr() const { return (*entity_).lock(); }
   EntityRef GetEntityRef() const { return (*entity_); }
   Entity& GetEntity() const { return *(*entity_).lock(); }
   void SetEntity(EntityPtr entity) { entity_ = entity; }

protected:
   void InitializeRecordFields() {
      AddRecordField("entity", entity_);
   }

private:
   dm::Boxed<EntityRef>    entity_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_COMPONENT_H
