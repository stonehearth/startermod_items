#include "pch.h"
#include <regex>
#include "stonehearth.h"
#include "entity.h"
#include "radiant_json.h"
#include "radiant_luabind.h"
#include "lua/radiant_lua.h"
#include "resources/res_manager.h"
#include "resources/exceptions.h"
#include "simulation/script/script_host.h"

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
Entity_GetNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
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


static std::string
GetLuaComponentUri(std::string name)
{
   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      modname = name.substr(0, offset);
      name = name.substr(offset + 1, std::string::npos);

      JSONNode const& manifest = resources::ResourceManager2::GetInstance().LookupManifest(modname);
      return manifest["components"][name].as_string();
   }
   // xxx: throw an exception...
   return "";
}

static luabind::object
Entity_GetLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
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

luabind::object
om::Stonehearth::GetComponent(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = Entity_GetNativeComponent(L, entity, name);
      if (!component.is_valid()) {
         component = Entity_GetLuaComponent(L, entity, name);
      }
   }
   return component;
}


static luabind::object
Entity_AddNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->AddComponent<om::Clas>(); \
      return luabind::object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return luabind::object();
}

static luabind::object
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
      // xxx: this should just be a generic scriphost!
      simulation::ScriptHost* host = luabind::object_cast<simulation::ScriptHost*>(globals(L)["native"]);
      ASSERT(host);
      object ctor = host->LuaRequire(uri);
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
   static std::regex entity_macro("^entity\\((.*), (.*)\\)$");
   std::smatch match;

   if (std::regex_match(entity_ref, match, entity_macro)) {
      InitEntity(entity, match[1], match[2], L);
   }
}

void Stonehearth::InitEntity(om::EntityPtr entity, std::string const& mod_name, std::string const& entity_name, lua_State* L)
{
   if (entity->GetModuleName().empty()) {
      entity->SetName(mod_name, entity_name);
   }

   try {
      std::string uri = resources::ResourceManager2::GetInstance().GetEntityUri(mod_name, entity_name);
      InitEntityByUri(entity, uri, L);
   } catch (resources::Exception &e) {
      std::ostringstream error;
      error << "failed to initialize entity(" << mod_name << ", " << entity_name << ") :"  << e.what();
      throw resources::Exception(error.str());
   }
}

void Stonehearth::InitEntityByUri(om::EntityPtr entity, std::string const& uri, lua_State* L)
{
   JSONNode const& node = resources::ResourceManager2::GetInstance().LookupJson(uri);
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
                  luabind::call_function<void>(extend, component, lua::JsonToLua(L, entry));
               }
            }
         }
      }
   }
}
