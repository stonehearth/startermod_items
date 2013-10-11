#include "pch.h"
#include <regex>
#include "stonehearth.h"
#include "entity.h"
#include "radiant_json.h"
#include "radiant_luabind.h"
#include "lua/radiant_lua.h"
#include "resources/res_manager.h"
#include "resources/exceptions.h"
#include "lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;

csg::Region3 Stonehearth::ComputeStandingRegion(const csg::Region3& r, int height)
{
   csg::Region3 standing;
   for (const csg::Cube3& c : r) {
      auto p0 = c.GetMin();
      auto p1 = c.GetMax();
      auto w = p1[0] - p0[0], h = p1[2] - p0[2];
      standing.Add(csg::Cube3(p0 - csg::Point3(0, 0, 1), p0 + csg::Point3(w, height, 0)));  // top
      standing.Add(csg::Cube3(p0 - csg::Point3(1, 0, 0), p0 + csg::Point3(0, height, h)));  // left
      standing.Add(csg::Cube3(p0 + csg::Point3(0, 0, h), p0 + csg::Point3(w, height, h + 1)));  // bottom
      standing.Add(csg::Cube3(p0 + csg::Point3(w, 0, 0), p0 + csg::Point3(w + 1, height, h)));  // right
   }
   standing.Optimize();

   return standing;
}


static luabind::object
GetNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<om::Clas>(); \
      if (!component) { \
         return luabind::object(); \
      } \
      return luabind::object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return luabind::object();
}

static luabind::object
GetNativeComponentData(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   dm::ObjectPtr obj = nullptr;
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<om::Clas>(); \
      if (!component) { \
         return luabind::object(); \
      } \
      obj = component; \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   if (obj) {
      JSONNode node = om::ObjectFormatter().ObjectToJson(obj);
      return lua::ScriptHost::JsonToLua(L, node);
   }
   return luabind::object();
}


static std::string
GetLuaComponentUri(std::string name)
{
   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      modname = name.substr(0, offset);
      name = name.substr(offset + 1, std::string::npos);

      json::ConstJsonObject manifest = res::ResourceManager2::GetInstance().LookupManifest(modname).GetNode();
      return manifest.getn("components").get<std::string>(name);
   }
   // xxx: throw an exception...
   return "";
}

static luabind::object
GetLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   om::LuaComponentsPtr component = entity->GetComponent<om::LuaComponents>();
   if (component) {
      om::DataBindingPtr db = component->GetLuaComponent(name);
      if (db) {
         return db->GetModelObject();
      }
   }
   return luabind::object();
}

static luabind::object
GetLuaComponentData(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   om::LuaComponentsPtr component = entity->GetComponent<om::LuaComponents>();
   if (component) {
      om::DataBindingPtr db = component->GetLuaComponent(name);
      if (db) {
         return lua::ScriptHost::JsonToLua(L, db->GetJsonData());
      }
   }
   return luabind::object();
}

luabind::object
om::Stonehearth::GetComponent(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = GetNativeComponent(L, entity, name);
      if (!component.is_valid()) {
         component = GetLuaComponent(L, entity, name);
      }
   }
   return component;
}
luabind::object
om::Stonehearth::GetComponentData(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = GetNativeComponentData(L, entity, name);
      if (!component.is_valid()) {
         component = GetLuaComponentData(L, entity, name);
      }
   }
   return component;
}


static luabind::object
AddNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<om::Clas>(); \
      if (!component) { \
         component = entity->AddComponent<om::Clas>(); \
         component->ExtendObject(json::ConstJsonObject(JSONNode())); \
      } \
      return luabind::object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return luabind::object();
}

static luabind::object
AddLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   using namespace luabind;

   object result;
   om::LuaComponentsPtr component = entity->AddComponent<om::LuaComponents>();
   om::DataBindingPtr data_binding = component->GetLuaComponent(name);
   if (data_binding) {
      result = data_binding->GetModelObject();
   } else {
      std::string uri = GetLuaComponentUri(name);
      object ctor = lua::ScriptHost::RequireScript(L, uri);
      if (ctor) {
         data_binding = component->AddLuaComponent(name);
         data_binding->SetDataObject(newtable(L));
         result = call_function<object>(ctor, om::EntityRef(entity), om::DataBindingRef(data_binding));
         data_binding->SetModelObject(result);
      }
   }
   return result;
}

luabind::object
om::Stonehearth::AddComponent(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = AddNativeComponent(L, entity, name);
      if (!component.is_valid()) {
         component = AddLuaComponent(L, entity, name);
         ASSERT(component.is_valid());
      }
   }
   return component;
}

void Stonehearth::InitEntity(om::EntityPtr entity, std::string const& uri, lua_State* L)
{
   entity->SetUri(uri);
   entity->SetDebugText(uri);

   JSONNode const& node = res::ResourceManager2::GetInstance().LookupJson(uri);
   auto i = node.find("components");
   if (i != node.end() && i->type() == JSON_NODE) {
      for (auto const& entry : *i) {
         // Native components...
   #define OM_OBJECT(Cls, lower) \
         if (entry.name() == #lower) { \
            auto component = entity->AddComponent<om::Cls>(); \
            component->ExtendObject(json::ConstJsonObject(entry)); \
            continue; \
         }
         OM_ALL_COMPONENTS
   #undef OM_OBJECT
         // Lua components...
         if (L) {
            luabind::object component = Stonehearth::AddComponent(L, entity, entry.name());
            if (luabind::type(component) != LUA_TNIL) {
               luabind::object extend = component["extend"];
               if (luabind::type(extend) == LUA_TFUNCTION) {
                  luabind::call_function<void>(extend, component, lua::ScriptHost::JsonToLua(L, entry));
               }
            }
         }
      }
   }
}
