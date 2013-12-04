#include "pch.h"
#include <regex>
#include "stonehearth.h"
#include "entity.h"
#include "lib/json/node.h"
#include "lib/lua/bind.h"
#include "resources/res_manager.h"
#include "resources/exceptions.h"
#include "lib/lua/script_host.h"
#include "om/object_formatter/object_formatter.h"

using namespace ::radiant;
using namespace ::radiant::om;
using namespace ::luabind;

static const std::regex entity_macro_regex__("^([^:\\\\/]+):([^\\\\/]+)$");

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

static object
GetNativeComponent(lua_State* L, EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<Clas>(); \
      if (!component) { \
         return object(); \
      } \
      return object(L, std::weak_ptr<Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return object();
}

static object
GetNativeComponentData(lua_State* L, EntityPtr entity, std::string const& name)
{
   dm::ObjectPtr obj = nullptr;
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<Clas>(); \
      if (!component) { \
         return object(); \
      } \
      obj = component; \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   if (obj) {
      JSONNode node = ObjectFormatter().ObjectToJson(obj);
      return lua::ScriptHost::JsonToLua(L, node);
   }
   return object();
}


static std::string
GetLuaComponentUri(std::string name)
{
   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      modname = name.substr(0, offset);
      name = name.substr(offset + 1, std::string::npos);

      json::Node manifest = res::ResourceManager2::GetInstance().LookupManifest(modname);
      return manifest.get_node("components").get<std::string>(name);
   }
   // xxx: throw an exception...
   return "";
}

static object
GetLuaComponent(lua_State* L, EntityPtr entity, std::string const& name)
{
   LuaComponentsPtr component = entity->GetComponent<LuaComponents>();
   if (component) {
      DataStorePtr db = component->GetLuaComponent(name);
      if (db) {
         return db->GetController();
      }
   }
   return object();
}

static object
GetLuaComponentData(lua_State* L, EntityPtr entity, std::string const& name)
{
   LuaComponentsPtr component = entity->GetComponent<LuaComponents>();
   if (component) {
      DataStorePtr db = component->GetLuaComponent(name);
      if (db) {
         // return lua::ScriptHost::JsonToLua(L, db->GetJsonData());
         // xxx: does this work?
         NOT_TESTED();
         return db->GetData().GetDataObject();
      }
   }
   return object();
}

object
Stonehearth::GetComponent(lua_State* L, EntityRef e, std::string name)
{
   object component;
   auto entity = e.lock();
   if (entity) {
      component = GetNativeComponent(L, entity, name);
      if (!component.is_valid()) {
         component = GetLuaComponent(L, entity, name);
      }
   }
   return component;
}

object
Stonehearth::GetComponentData(lua_State* L, EntityRef e, std::string name)
{
   object component;
   auto entity = e.lock();
   if (entity) {
      component = GetNativeComponentData(L, entity, name);
      if (!component.is_valid()) {
         component = GetLuaComponentData(L, entity, name);
      }
   }
   return component;
}

object
Stonehearth::AddComponentData(lua_State* L, EntityRef e, std::string name)
{
   object o = GetComponentData(L, e, name);
   if (!o.is_valid() || type(o) == LUA_TNIL) {
      o = newtable(L);
      SetComponentData(L, e, name, o);
   }
   return o;
}

static object
AddNativeComponent(lua_State* L, EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<Clas>(); \
      if (!component) { \
         component = entity->AddComponent<Clas>(); \
         component->ExtendObject(json::Node(JSONNode())); \
      } \
      return object(L, std::weak_ptr<Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return object();
}

static bool
SetNativeComponentData(lua_State* L, EntityPtr entity, std::string const& name, object data)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->AddComponent<Clas>(); \
      component->ExtendObject(lua::ScriptHost::LuaToJson(L, data)); \
      return true; \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return false;
}

static object
AddLuaComponent(lua_State* L, EntityPtr entity, std::string const& name)
{
   object result;
   LuaComponentsPtr component = entity->AddComponent<LuaComponents>();
   DataStorePtr data_store = component->GetLuaComponent(name);
   if (data_store) {
      result = data_store->GetController();
   } else {
      std::string uri = GetLuaComponentUri(name);
      object ctor = lua::ScriptHost::RequireScript(L, uri);
      if (ctor) {
         data_store = component->AddLuaComponent(name);
         data_store->SetData(lua::DataObject(newtable(L)));
         result = call_function<object>(ctor, EntityRef(entity), data_store);
         data_store->SetController(lua::ControllerObject(result));
      }
   }
   return result;
}

static void
SetLuaComponentData(lua_State* L, EntityPtr entity, std::string const& name, object data)
{
   object result;
   LuaComponentsPtr component = entity->AddComponent<LuaComponents>();
   DataStorePtr data_store = component->AddLuaComponent(name);
   data_store->SetData(lua::DataObject(data));
}

object
Stonehearth::AddComponent(lua_State* L, EntityRef e, std::string name)
{
   object component;
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

void
Stonehearth::SetComponentData(lua_State* L, EntityRef e, std::string name, object data)
{
   object component;
   auto entity = e.lock();
   if (entity) {
      if (!SetNativeComponentData(L, entity, name, data)) {
         SetLuaComponentData(L, entity, name, data);
      }
   }
}

void Stonehearth::InitEntity(EntityPtr entity, std::string const& uri, lua_State* L)
{
   ASSERT(L);
   L = lua::ScriptHost::GetCallbackThread(L);
   bool is_server = object_cast<bool>(globals(L)["radiant"]["is_server"]);

   entity->SetUri(uri);
   entity->SetDebugText(uri);

   JSONNode const& node = res::ResourceManager2::GetInstance().LookupJson(uri);
   auto i = node.find("components");
   if (i != node.end() && i->type() == JSON_NODE) {
      for (auto const& entry : *i) {
         // Native components...
   #define OM_OBJECT(Cls, lower) \
         if (entry.name() == #lower) { \
            auto component = entity->AddComponent<Cls>(); \
            component->ExtendObject(json::Node(entry)); \
            continue; \
         }
         OM_ALL_COMPONENTS
   #undef OM_OBJECT

         // Lua components...
         object component_data = lua::ScriptHost::JsonToLua(L, entry);
         if (is_server) {
            object component = Stonehearth::AddComponent(L, entity, entry.name());
            if (type(component) != LUA_TNIL) {
               object extend = component["extend"];
               if (type(extend) == LUA_TFUNCTION) {
                  call_function<void>(extend, component, component_data);
               }
            }
         } else {
            SetLuaComponentData(L, entity, entry.name(), component_data);
         }
      }
   }
   // go through again and call the post create function...
   if (is_server) {
      auto lua_components = entity->GetComponent<LuaComponents>();
      if (lua_components) {
         for (auto const& entry : lua_components->GetComponentMap()) {
            object component = entry.second->GetController();
            ASSERT(component.is_valid() && type(component) != LUA_TNIL);

            object on_created = component["on_created"];
            if (type(on_created) == LUA_TFUNCTION) {
               call_function<void>(on_created, component);
            }
         }
      }
   }

   // xxx: refaactor me!!!111!
   if (L) {
      json::Node n(node);
      std::string init_script = n.get<std::string>("init_script", "");
      if (!init_script.empty()) {
         try {        
            object fn = lua::ScriptHost::RequireScript(L, init_script);
            if (!fn.is_valid() || type(fn) != LUA_TFUNCTION) {
               LOG(WARNING) << "failed to load init script " << init_script << "... skipping.";
            } else {
               call_function<void>(fn, EntityRef(entity));
            }
         } catch (std::exception &e) {
            LOG(WARNING) << "failed to run init script for " << uri << ": " << e.what();
         }
      }
   }
}
