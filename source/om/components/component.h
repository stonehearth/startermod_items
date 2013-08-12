#ifndef _RADIANT_OM_COMPONENT_H
#define _RADIANT_OM_COMPONENT_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "om/entity.h"
#include "radiant_json.h"

BEGIN_RADIANT_OM_NAMESPACE
   
class Component : public dm::Record
{
public:
   Component() { }

   virtual void ExtendObject(json::ConstJsonObject const& obj) { };
   virtual void Describe(std::ostringstream& os) const { };

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
std::ostream& operator<<(std::ostream& os, const Component& o);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_COMPONENT_H
