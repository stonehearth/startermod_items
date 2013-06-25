#ifndef _RADIANT_OM_LUA_COMPONENTS_H
#define _RADIANT_OM_LUA_COMPONENTS_H

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

class LuaComponent : public dm::Object
{
public:
   LuaComponent();
   DEFINE_OM_OBJECT_TYPE_NO_CONS(LuaComponent, lua_component);
   static void RegisterLuaType(struct lua_State* L);
   std::string ToString() const;

   void SetLuaObject(std::string const& name, luabind::object obj);
   luabind::object GetLuaObject() const { return obj_; }

   JSONNode ToJson() const;

#if 0
   dm::Guard Trace(const char* reason, std::function<void(JSONNode const &)> cb) const {
      return TraceObjectChanges(reason, [=]() {
         cb(json_);
      });
   }
#endif

   std::ostream& Log(std::ostream& os, std::string indent) const override {
      return (os << "lua_component [oid:" << GetObjectId() << "]" << std::endl);
   }

private:
   void CloneObject(Object* c, dm::CloneMapping& mapping) const override {
      LuaComponent& copy = static_cast<LuaComponent&>(*c);
      mapping.objects[GetObjectId()] = copy.GetObjectId();
      ASSERT(false); // xxx: no way to clone this.  get rid of cloning!!
      copy.obj_ = obj_;
   }

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
static std::ostream& operator<<(std::ostream& os, const LuaComponent& o) { return (os << o.ToString()); }

class LuaComponents : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(LuaComponents, lua_components);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   std::string ToString() const;

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

static std::ostream& operator<<(std::ostream& os, const LuaComponents& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_LUA_COMPONENTS_H
