#include "radiant.h"
#include <regex>
#include "stonehearth.h"
#include "entity.h"
#include "components/component.h"
#include "lib/json/node.h"
#include "lib/lua/bind.h"
#include "resources/res_manager.h"
#include "resources/exceptions.h"
#include "lib/lua/script_host.h"
#include "om/components/data_store.ridl.h"
#include "om/components/data_store_ref_wrapper.h"

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
      object activate_fn = component["activate"];
      if (type(activate_fn) == LUA_TFUNCTION) {
         activate_fn(component, om::EntityRef(entity));
      }                  
   } catch (std::exception const& e) {
      lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
      scriptHost->ReportCStackThreadException(L, e);
   }
}

object
Stonehearth::AddComponent(lua_State* L, EntityRef e, const char* name)
{
   object component = GetComponent(L, e, name);
   if (!component) {
      auto entity = e.lock();
      if (entity) {
         om::ComponentPtr obj = entity->AddComponent(name);
         if (obj) {
            lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
            obj->LoadFromJson(JSONNode());
            component = scriptHost->CastObjectToLua(obj);
         } else {
            // We're manually allocating a datastore (because we can't tell if we're on the server
            // or the client).  The entity will manually delete them when being destroyed.
            om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
            datastore->SetData(luabind::newtable(L));
            component = datastore->CreateController(datastore, "components", name, om::DataStore::IS_CPP_MANAGED);
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
Stonehearth::RemoveComponent(lua_State* L, EntityRef e, const char* name)
{
   auto entity = e.lock();
   if (entity) {
      entity->RemoveComponent(name);
   }
}

object
Stonehearth::SetComponentData(lua_State* L, EntityRef e, const char* name, object data)
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
         // We're manually allocating a datastore (because we can't tell if we're on the server
         // or the client).  The entity will manually delete them when being destroyed.
         om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
         datastore->SetData(luabind::newtable(L));
         component = datastore->CreateController(datastore, "components", name, om::DataStore::IS_CPP_MANAGED);
         if (component) {
            entity->AddLuaComponent(name, datastore);
            SetEntityForComponent(L, component, entity, data);
         }
      }
   }
   return component;
}

object
Stonehearth::GetComponent(lua_State* L, EntityRef e, const char* name)
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
               component = luabind::object(L, om::DataStoreRefWrapper(datastore));
            }
         }
      }
   }
   return component;
}

void
Stonehearth::InitEntity(EntityPtr entity, const char* uri, lua_State* L)
{
   ASSERT(L);

   lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
   entity->SetUri(uri);
   if (uri[0]) {
      res::ResourceManager2::GetInstance().LookupJson(uri, [&](const JSONNode& node) {
         auto i = node.find("components");
         if (i != node.end() && i->type() == JSON_NODE) {
            for (auto const& entry : *i) {
               std::string const& component_name = entry.name();
               ComponentPtr component = entity->AddComponent(component_name.c_str());
               if (component) {
                  component->LoadFromJson(json::Node(entry));
               } else {
                  om::DataStorePtr datastore = entity->GetStore().AllocObject<om::DataStore>();
                  datastore->SetData(luabind::newtable(L));
                  object lua_component = datastore->CreateController(datastore, "components", component_name, om::DataStore::IS_CPP_MANAGED);
                  if (lua_component.is_valid()) {
                     entity->AddLuaComponent(component_name.c_str(), datastore);
                     // XXX: don't call initialize till they're all there!
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
                  object fn = lua::ScriptHost::GetScriptHost(L)->RequireScript(init_script);
                  if (!fn.is_valid() || type(fn) != LUA_TFUNCTION) {
                     E_LOG(3) << "failed to load init script " << init_script << "... skipping.";
                  } else {
                     fn(EntityRef(entity));
                  }
               } catch (std::exception &e) {
                  E_LOG(3) << "failed to run init script for " << uri << ": " << e.what();
               }
            }
         }
      });
   }
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
         E_LOG(5) << "restoring entity for component datastore " << datastore->GetObjectId();
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
