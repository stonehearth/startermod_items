#ifndef _RADIANT_OM_ATTRIBUTES_H
#define _RADIANT_OM_ATTRIBUTES_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"
#include "csg/cube.h"

BEGIN_RADIANT_OM_NAMESPACE

class Attributes : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Attributes);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   int GetAttribute(std::string name) const;
   bool HasAttribute(std::string name) const;
   void SetAttribute(std::string name, int value);

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("attributes",  attributes_);
   }

public:
   dm::Map<std::string, int>    attributes_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ATTRIBUTES_H
