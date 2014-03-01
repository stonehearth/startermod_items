#include <libjson.h>
#include "lib/lua/pch.h"
#include "open.h"
#include "om/all_objects.h"
#include "om/all_components.h"
#include "om/object_formatter/object_formatter.h"
#include "lib/lua/script_host.h"
#include "om/lua/lua_om.h"

using namespace ::radiant;
using namespace ::radiant::om;
using namespace luabind;

luabind::object json_to_lua(dm::Store const& store, lua_State* L, JSONNode const& node)
{
   try {
      if (node.type() == JSON_STRING) {
         dm::ObjectPtr obj = store.FetchObject<dm::Object>(node.as_string());
         if (obj) {

            if (obj->GetObjectType() == DataStoreObjectType) {
               return object(L, std::static_pointer_cast<DataStore>(obj));
            }
#define OM_OBJECT(Cls, lower) \
            case Cls ## ObjectType: \
               return object(L, std::weak_ptr<Cls>(std::static_pointer_cast<Cls>(obj)));

            switch(obj->GetObjectType()) {
               OM_ALL_COMPONENTS
               OM_ALL_OBJECTS
            }

#undef OM_OBJECT

            LUA_LOG(1) << "unrecognized object type '" << obj->GetObjectClassNameLower() << "' in json_to_lua!";
         }
      }
   } catch (std::exception& e) {
      LUA_LOG(1) << "critical error in json_to_lua: " << e.what();
      lua::ScriptHost::ReportCStackException(L, e);
   }
   return luabind::object();
}

void lua::om::open(lua_State* L)
{
   radiant::om::RegisterLuaTypes(L);
}

void lua::om::register_json_to_lua_objects(lua_State* L, dm::Store& dm)
{
   lua::ScriptHost* sh = ScriptHost::GetScriptHost(L);
   sh->AddJsonToLuaConverter([&dm](lua_State* L, JSONNode const& node) {
      return json_to_lua(dm, L, node);
   });
}
