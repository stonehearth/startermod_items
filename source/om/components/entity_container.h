#ifndef _RADIANT_OM_ENTITY_CONTAINER_H
#define _RADIANT_OM_ENTITY_CONTAINER_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "namespace.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Entity;

class EntityContainer : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(EntityContainer);

   typedef dm::Map<dm::ObjectId, std::weak_ptr<Entity>> Container;

   const Container& GetChildren() const { return children_; }

   void AddChild(om::EntityRef child);
   void RemoveChild(om::EntityRef child);

private:
   void InitializeRecordFields() override;

public:
   Container         children_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ENTITY_CONTAINER_H
