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
using namespace ::luabind;

static const std::regex entity_macro_regex__("^([^\\.\\\\/]+)\\.([^\\\\/]+)$");

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
Entity_GetNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<om::Clas>(); \
      if (!component) { \
         return object(); \
      } \
      return object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
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

      json::ConstJsonObject manifest = res::ResourceManager2::GetInstance().LookupManifest(modname).GetNode();
      return manifest.getn("components").get<std::string>(name);
   }
   // xxx: throw an exception...
   return "";
}

static object
Entity_GetLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   om::LuaComponentsPtr component = entity->GetComponent<om::LuaComponents>();
   if (component) {
      om::DataBindingPtr db = component->GetLuaComponent(name);
      if (db) {
         return db->GetModelObject();
      }
   }
   return object();
}

object
om::Stonehearth::GetComponent(lua_State* L, om::EntityRef e, std::string name)
{
   object component;
   auto entity = e.lock();
   if (entity) {
      component = Entity_GetNativeComponent(L, entity, name);
      if (!component.is_valid()) {
         component = Entity_GetLuaComponent(L, entity, name);
      }
   }
   return component;
}


static object
Entity_AddNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->AddComponent<om::Clas>(); \
      return object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return object();
}

static object
Entity_AddLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
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

object
om::Stonehearth::AddComponent(lua_State* L, om::EntityRef e, std::string name)
{
   object component;
   auto entity = e.lock();
   if (entity) {
      component = Entity_AddNativeComponent(L, entity, name);

      if (!component.is_valid()) {
         component = Entity_AddLuaComponent(L, entity, name);
         ASSERT(component.is_valid());
      }
   }
   return component;
}

void Stonehearth::InitEntityByRef(om::EntityPtr entity, std::string const& entity_ref, lua_State* L)
{   
   std::smatch match;

   if (std::regex_match(entity_ref, match, entity_macro_regex__)) {
      InitEntity(entity, match[1], match[2], L);
   }
}

void Stonehearth::InitEntity(om::EntityPtr entity, std::string const& mod_name, std::string const& entity_name, lua_State* L)
{
   if (entity->GetModuleName().empty()) {
      entity->SetName(mod_name, entity_name);
   }

   try {
      std::string uri = res::ResourceManager2::GetInstance().GetEntityUri(mod_name, entity_name);
      InitEntityByUri(entity, uri, L);
   } catch (res::Exception &e) {
      std::ostringstream error;
      error << "failed to initialize entity " << mod_name << "." << entity_name << " "  << e.what();
      throw res::Exception(error.str());
   }
}

void Stonehearth::InitEntityByUri(om::EntityPtr entity, std::string const& uri, lua_State* L)
{
   if (L) {
      L = lua::ScriptHost::GetCallbackThread(L);
   }

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
            object component = Stonehearth::AddComponent(L, entity, entry.name());
            if (type(component) != LUA_TNIL) {
               object extend = component["extend"];
               if (type(extend) == LUA_TFUNCTION) {
                  call_function<void>(extend, component, lua::ScriptHost::JsonToLua(L, entry));
               }
            }
         }
      }
   }
   // xxx: refaactor me!!!111!
   if (L) {
      json::ConstJsonObject n(node);
      std::string init_script = n.get<std::string>("init_script");
      if (!init_script.empty()) {
         try {        
            object fn = lua::ScriptHost::RequireScript(L, init_script);
            if (!fn.is_valid() || type(fn) != LUA_TFUNCTION) {
               LOG(WARNING) << "failed to load init script " << init_script << "... skipping.";
            } else {
               call_function<void>(fn, om::EntityRef(entity));
            }
         } catch (std::exception &e) {
            LOG(WARNING) << "failed to run init script for " << uri << ": " << e.what();
         }
      }
   }
}
