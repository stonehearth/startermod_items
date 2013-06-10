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

#if 0
class LuaStoreString : public dm::Boxed<std::string>
{
};

class LuaStoreTable : public dm::Map<std::string, dm::ObjectId>
{
public:
   luabind::object GetLua(std::string const& name);

   dm::ObjectPtr Get(std::string const& name);
   template <class T> std::shared_ptr<T> Get(std::string const& name);

   void Set(std::string const& name, dm::ObjectPtr obj);
   void SetLua(std::string const& name, luabind::object value);

};
#endif

class LuaComponent : public dm::Object
{
public:
   DEFINE_OM_OBJECT_TYPE(LuaComponent);
   static void RegisterLuaType(struct lua_State* L);
   std::string ToString() const;

   void SetLuaObject(luabind::object obj) { obj_ = obj; }
   luabind::object GetLuaObject() const { return obj_; }
   JSONNode const & GetJsonData() const { return json_; }
   void SaveJsonData(std::string const& data);

   dm::Guard Trace(const char* reason, std::function<void(JSONNode const &)> cb) const {
      return TraceObjectChanges(reason, [=]() {
         cb(json_);
      });
   }

   std::ostream& Log(std::ostream& os, std::string indent) const override {
      return (os << "lua_component [oid:" << GetObjectId() << "]" << std::endl);
   }

private:
   void CloneObject(Object* c, dm::CloneMapping& mapping) const override {
      LuaComponent& copy = static_cast<LuaComponent&>(*c);
      mapping.objects[GetObjectId()] = copy.GetObjectId();
      copy.json_ = json_;
      copy.obj_ = obj_;
   }

protected:
   void SaveValue(const dm::Store& store, Protocol::Value* msg) const override {
      dm::SaveImpl<std::string>::SaveValue(store, msg, json_.write());
   }
   void LoadValue(const dm::Store& store, const Protocol::Value& msg) override {
      std::string json;
      dm::SaveImpl<std::string>::LoadValue(store, msg, json);
      json_ = libjson::parse_unformatted(json);      
   }

private:
   JSONNode             json_;
   luabind::object      obj_;
};
typedef std::shared_ptr<LuaComponent> LuaComponentPtr;
typedef std::weak_ptr<LuaComponent> LuaComponentRef;
static std::ostream& operator<<(std::ostream& os, const LuaComponent& o) { return (os << o.ToString()); }

class LuaComponents : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(LuaComponents);
   void ExtendObject(json::ConstJsonObject const& obj) override;

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   std::string ToString() const;

   LuaComponentPtr GetLuaComponent(const char* name) const;
   LuaComponentPtr AddLuaComponent(const char* name);

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
