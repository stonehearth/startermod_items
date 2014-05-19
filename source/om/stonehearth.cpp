#include "pch.h"
#include <regex>
#include "stonehearth.h"
#include "entity.h"
#include "lib/json/node.h"
#include "lib/lua/bind.h"
#include "resources/res_manager.h"
#include "resources/exceptions.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;
using namespace ::luabind;

static const std::regex entity_macro_regex__("^([^:\\\\/]+):([^\\\\/]+)$");

#define E_LOG(level)    LOG(om.entity, level)

static void
TriggerPostCreate(lua::ScriptHost *scriptHost, om::EntityPtr entity)
{
   lua_State* L = scriptHost->GetInterpreter();
   luabind::object e(L, std::weak_ptr<om::Entity>(entity));
   luabind::object evt(L, luabind::newtable(L));
   evt["entity"] = e;
   scriptHost->TriggerOn(e, "radiant:entity:post_create", evt);
   scriptHost->Trigger("radiant:entity:post_create", evt);
}

std::string
Stonehearth::GetLuaComponentUri(dm::Store const& store, std::string const& name)
{
   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      modname = name.substr(0, offset);
      std::string compName = name.substr(offset + 1, std::string::npos);

      bool isServer = object_cast<bool>(globals(store.GetInterpreter())["radiant"]["is_server"]);
      const char* key = isServer ? "components" : "client_components" ;

      std::string result;
      res::ResourceManager2::GetInstance().LookupManifest(modname, [&result, &key, &compName](const res::Manifest& manifest) {
         result = manifest.get_node(key).get<std::string>(compName, "");
      });
      return result;
   }
   // xxx: throw an exception...
   return "";
}


luabind::object
Stonehearth::ConstructLuaComponent(lua::ScriptHost* scriptHost, om::EntityPtr entity, std::string const& uri, luabind::object json, om::DataStorePtr datastore)
{
   object component;

   if (entity) {
      lua_State* L = scriptHost->GetInterpreter();

      ASSERT(!uri.empty());
      ASSERT(json);
      ASSERT(datastore);

      try {
         object ctor = lua::ScriptHost::RequireScript(L, uri);
         if (ctor) {
            component = ctor();
            if (component) {
               datastore->SetController(lua::ControllerObject(uri, component));
               component["__saved_variables"] = datastore;
               object init_fn_obj = component["initialize"];
               if (type(init_fn_obj) == LUA_TFUNCTION) {
                  init_fn_obj(component, om::EntityRef(entity), json, datastore);
               }                  
            }
         }
      } catch (std::exception const& e) {
         scriptHost->ReportCStackThreadException(L, e);
      }
   }
   return component;
}

object
Stonehearth::AddComponent(lua_State* L, EntityRef e, std::string const& name)
{
   object component = GetComponent(L, e, name);
   if (!component) {
      auto entity = e.lock();
      if (entity) {
         lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
         om::ComponentPtr obj = entity->AddComponent(name);
         if (obj) {
            obj->LoadFromJson(JSONNode());
            component = scriptHost->CastObjectToLua(obj);
         } else {
            std::string uri = GetLuaComponentUri(entity->GetStore(), name);
            if (!uri.empty()) {
               om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
               datastore->SetData(luabind::newtable(L));
               component = ConstructLuaComponent(scriptHost, entity, uri, luabind::newtable(L), datastore);
               entity->AddLuaComponent(name, datastore);
            }
         }
      }
   }
   return component;
}

void
Stonehearth::RemoveComponent(lua_State* L, EntityRef e, std::string const& name)
{
   auto entity = e.lock();
   if (entity) {
      entity->RemoveComponent(name);
   }
}

object
Stonehearth::SetComponentData(lua_State* L, EntityRef e, std::string const& name, object data)
{
   object component;
   om::EntityPtr entity = e.lock();
   if (entity) {
      lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
      entity->RemoveComponent(name);
      om::ComponentPtr obj = entity->AddComponent(name);
      if (obj) {
         obj->LoadFromJson(scriptHost->LuaToJson(data));
         component = scriptHost->CastObjectToLua(obj);
      } else {
         std::string uri = GetLuaComponentUri(entity->GetStore(), name);
         if (!uri.empty()) {
            om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
            datastore->SetData(luabind::newtable(L));

            component = ConstructLuaComponent(scriptHost, entity, uri, data, datastore);
            ASSERT(component);
            entity->AddLuaComponent(name, datastore);
         }
      }
   }
   return component;
}

object
Stonehearth::GetComponent(lua_State* L, EntityRef e, std::string const& name)
{
   object component;
   lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);

   auto entity = e.lock();
   if (entity) {
      dm::ObjectPtr obj = entity->GetComponent(name);
      if (obj) {
         component = scriptHost->CastObjectToLua(obj);
      } else {
         om::DataStorePtr datastore = entity->GetLuaComponent(name);
         if (datastore) {
            component = datastore->GetController().GetLuaObject();
            if (!component) {
               component = luabind::object(L, datastore);
            }
         }
      }
   }
   return component;
}

void
Stonehearth::InitEntity(EntityPtr entity, std::string const& uri, lua_State* L)
{
   ASSERT(L);

   lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);

   entity->SetUri(uri);
   res::ResourceManager2::GetInstance().LookupJson(uri, [&](const JSONNode& node) {
      auto i = node.find("components");
      if (i != node.end() && i->type() == JSON_NODE) {
         for (auto const& entry : *i) {
            std::string const& component_name = entry.name();
            ComponentPtr component = entity->AddComponent(component_name);
            if (component) {
               component->LoadFromJson(json::Node(entry));
            } else {
               std::string uri = GetLuaComponentUri(entity->GetStore(), component_name);
               if (!uri.empty()) {
                  om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
                  datastore->SetData(luabind::newtable(L));

                  object component_data = lua::ScriptHost::JsonToLua(L, entry);
                  object lua_component = ConstructLuaComponent(scriptHost, entity, uri, component_data, datastore);
                  ASSERT(lua_component && lua_component.is_valid());
                  entity->AddLuaComponent(component_name, datastore);
               }
            }
         }
      }

      // Initialize after all components have been added
      for (auto const& entry : entity->GetComponents()) {
         entry.second->Initialize();
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
   TriggerPostCreate(scriptHost, entity);
}

void
Stonehearth::RestoreLuaComponents(lua::ScriptHost* scriptHost, EntityPtr entity)
{
   auto restore = [scriptHost, entity](JSONNode const& data) {
      lua_State* L = scriptHost->GetInterpreter();

      auto const components = data.find("components"), end = data.end();
      for (auto const& entry : entity->GetLuaComponents()) {
         std::string component_name = entry.first;
         om::DataStorePtr datastore = entry.second;

         JSONNode json;
         if (components != end) {
            auto const j = components->find(component_name);
            if (j != components->end()) {
               json = *j;
            }
         }

         std::string uri = GetLuaComponentUri(entity->GetStore(), component_name);
         if (!uri.empty()) {
            object lua_component = ConstructLuaComponent(scriptHost, entity, uri, lua::ScriptHost::JsonToLua(L, json), datastore);
            ASSERT(lua_component && lua_component.is_valid());
            entity->AddLuaComponent(component_name, datastore);
         }
      }
      TriggerPostCreate(scriptHost, entity);
   };

   for (auto const& entry : entity->GetComponents()) {
      entry.second->Initialize();
   }
   std::string const& uri = entity->GetUri();
   if (!uri.empty()) {
      res::ResourceManager2::GetInstance().LookupJson(uri, restore);
   } else {
      restore(JSONNode());
   }
}
