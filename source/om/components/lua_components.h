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

class LuaComponent : public dm::Object
{
public:
   LuaComponent();
   DEFINE_OM_OBJECT_TYPE_NO_CONS(LuaComponent, lua_component);

   void SetLuaObject(std::string const& name, luabind::object obj);
   luabind::object GetLuaObject() const { return obj_; }

   JSONNode ToJson() const;
   std::string GetName() const { return name_; }

#if 0
   dm::Guard Trace(const char* reason, std::function<void(JSONNode const &)> cb) const {
      return TraceObjectChanges(reason, [=]() {
         cb(json_);
      });
   }
#endif
protected:
   void SaveValue(const dm::Store& store, Protocol::Value* msg) const override;
   void LoadValue(const dm::Store& store, const Protocol::Value& msg) override;

private:
   std::string          name_;
   luabind::object      obj_;
   mutable JSONNode     cached_json_;
   mutable bool         cached_json_valid_;
};

typedef std::shared_ptr<LuaComponent> LuaComponentPtr;
typedef std::weak_ptr<LuaComponent> LuaComponentRef;
std::ostream& operator<<(std::ostream& os, const LuaComponent& o);

class LuaComponents : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(LuaComponents, lua_components);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   LuaComponentPtr GetLuaComponent(std::string name) const;
   LuaComponentPtr AddLuaComponent(std::string name);

   typedef dm::Map<std::string, LuaComponentPtr> ComponentMap;
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
