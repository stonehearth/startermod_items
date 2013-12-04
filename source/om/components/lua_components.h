#ifndef _RADIANT_OM_LUA_COMPONENTS_H
#define _RADIANT_OM_LUA_COMPONENTS_H

#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/object_enums.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"
#include "csg/cube.h"

BEGIN_RADIANT_OM_NAMESPACE

class LuaComponents : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(LuaComponents, lua_components);
   void ExtendObject(json::Node const& obj) override;

   DataStorePtr GetLuaComponent(std::string name) const;
   DataStorePtr AddLuaComponent(std::string name);

   typedef dm::Map<std::string, DataStorePtr> ComponentMap;
   ComponentMap const& GetComponentMap() const { return lua_components_; }

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("components", lua_components_);
   }

public:
   ComponentMap   lua_components_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_LUA_COMPONENTS_H
