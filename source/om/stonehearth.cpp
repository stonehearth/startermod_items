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

#define E_LOG(level)    LOG(om.entity, level)

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

static std::string
GetLuaComponentUri(std::string name)
{
   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      modname = name.substr(0, offset);
      name = name.substr(offset + 1, std::string::npos);

      std::string result;
      res::ResourceManager2::GetInstance().LookupManifest(modname, [&](const res::Manifest& manifest) {
         result = manifest.get_node("components").get<std::string>(name);
      });
      return result;
   }
   // xxx: throw an exception...
   return "";
}

static object
ConstructLuaComponent(lua::ScriptHost* scriptHost, std::string const& name, om::EntityRef entity, luabind::object component_data)
{
   object obj;
   lua_State* L = scriptHost->GetInterpreter();

   try {
      std::string uri = GetLuaComponentUri(name);
      object ctor = lua::ScriptHost::RequireScript(L, uri);
      if (ctor) {
         obj = ctor();
         if (obj) {
            object __create = obj["__create"];
            if (type(__create) == LUA_TFUNCTION) {
               call_function<void>(__create, obj, entity, component_data);
            }                  
         }
      }

   } catch (std::exception const& e) {
      scriptHost->ReportCStackThreadException(L, e);
   }
   return obj;
}

object
Stonehearth::AddComponent(lua_State* L, EntityRef e, std::string name)
{
   object component = GetComponent(L, e, name);
   if (!component) {
      auto entity = e.lock();
      if (entity) {
         lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
         om::ComponentPtr obj = entity->AddComponent(name);
         if (obj) {
            component = scriptHost->CastObjectToLua(obj);
         } else {
            component = ConstructLuaComponent(scriptHost, name, e, luabind::newtable(L));
            if (component) {
               entity->AddLuaComponent(name, component);
            }
         }
      }
   }
   return component;
}

object
Stonehearth::SetComponentData(lua_State* L, EntityRef e, std::string name, object data)
{
   object result;
   // xxx: this is somewhat gross.  we should simply bind a different function in the
   // client vs the server!
   bool is_server = object_cast<bool>(globals(L)["radiant"]["is_server"]);
   if (is_server) {
      om::EntityPtr entity = e.lock();
      if (entity) {
         lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
         entity->RemoveComponent(name);
         om::ComponentPtr obj = entity->AddComponent(name);
         if (obj) {
            obj->ExtendObject(scriptHost->LuaToJson(data));
            result = scriptHost->CastObjectToLua(obj);
         } else {
            result = ConstructLuaComponent(scriptHost, name, e, data);
            if (result) {
               entity->AddLuaComponent(name, result);
            }
         }
      }
   } else {
      NOT_YET_IMPLEMENTED();
   }
   return result;
}

object
Stonehearth::GetComponent(lua_State* L, EntityRef e, std::string name)
{
   object component;
   lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);

   auto entity = e.lock();
   if (entity) {
      dm::ObjectPtr obj = entity->GetComponent(name);
      if (obj) {
         component = scriptHost->CastObjectToLua(obj);
      } else {
         component = entity->GetLuaComponent(name);
      }
   }
   return component;
}

object
Stonehearth::GetComponentData(lua_State* L, EntityRef e, std::string name)
{
   return GetComponent(L, e, name);
}

void
Stonehearth::InitEntity(EntityPtr entity, std::string const& uri, lua_State* L)
{
   ASSERT(L);

   lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
   bool is_server = object_cast<bool>(globals(L)["radiant"]["is_server"]);

   entity->SetUri(uri);
   res::ResourceManager2::GetInstance().LookupJson(uri, [&](const JSONNode& node) {
      auto i = node.find("components");
      if (i != node.end() && i->type() == JSON_NODE) {
         for (auto const& entry : *i) {
            std::string const& component_name = entry.name();
            ComponentPtr component = entity->AddComponent(component_name);
            if (component) {
               component->ExtendObject(json::Node(entry));
            } else {
               object lua_component;
               if (is_server) {
                  object component_data = lua::ScriptHost::JsonToLua(L, entry);
                  lua_component = ConstructLuaComponent(scriptHost, component_name, entity, component_data);
               } else {
                  lua_component = lua::ScriptHost::JsonToLua(L, entry);
               }
               if (lua_component) {
                  entity->AddLuaComponent(component_name, lua_component);
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
                  E_LOG(3) << "failed to load init script " << init_script << "... skipping.";
               } else {
                  call_function<void>(fn, EntityRef(entity));
               }
            } catch (std::exception &e) {
               E_LOG(3) << "failed to run init script for " << uri << ": " << e.what();
            }
         }
      }
   });
}
