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

void
Stonehearth::SetEntityForComponent(lua_State* L, luabind::object component, om::EntityPtr entity, luabind::object json)
{
   try {
      object initialize_fn = component["initialize"];
      if (type(initialize_fn) == LUA_TFUNCTION) {
         initialize_fn(component, om::EntityRef(entity), json);
      }                  
   } catch (std::exception const& e) {
      lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
      scriptHost->ReportCStackThreadException(L, e);
   }
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
            om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
            datastore->SetData(luabind::newtable(L));
            component = datastore->CreateController(datastore, "components", name);
            if (component) {
               entity->AddLuaComponent(name, datastore);
               SetEntityForComponent(L, component, entity, luabind::newtable(L));
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
         om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
         datastore->SetData(luabind::newtable(L));
         component = datastore->CreateController(datastore, "components", name);
         if (component) {
            entity->AddLuaComponent(name, datastore);
            SetEntityForComponent(L, component, entity, data);
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
            component = datastore->GetController();
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
               om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
               datastore->SetData(luabind::newtable(L));
               object lua_component = datastore->CreateController(datastore, "components", component_name);
               if (lua_component.is_valid()) {
                  entity->AddLuaComponent(component_name, datastore);
                  SetEntityForComponent(L, lua_component, entity, lua::ScriptHost::JsonToLua(L, entry));
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
         luabind::object lua_component = datastore->GetController();
         ASSERT(lua_component && lua_component.is_valid());

         // XXX: LEGACY!!!
         SetEntityForComponent(L, lua_component, entity, lua::ScriptHost::JsonToLua(L, json));
      }
      TriggerPostCreate(scriptHost, entity);
   };

   for (auto const& entry : entity->GetComponents()) {
      entry.second->Initialize(); // xxx: restore, right?
   }

   std::string const& uri = entity->GetUri();
   if (!uri.empty()) {
      res::ResourceManager2::GetInstance().LookupJson(uri, restore);
   } else {
      restore(JSONNode());
   }
}
