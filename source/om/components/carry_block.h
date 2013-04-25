#ifndef _RADIANT_OM_CARRY_BLOCK_H
#define _RADIANT_OM_CARRY_BLOCK_H

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

class CarryBlock : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(CarryBlock);

   CarryBlock() { }

   om::EntityRef GetCarrying() const { return *carrying_; }
   void SetCarrying(om::EntityRef obj) { carrying_ = obj; }
   dm::Guard TraceCarrying(const char* reason, std::function<void()> fn) { return carrying_.TraceObjectChanges(reason, fn); }

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("carrying", carrying_);
   }

public:
   dm::Boxed<om::EntityRef>      carrying_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_CARRY_BLOCK_H
