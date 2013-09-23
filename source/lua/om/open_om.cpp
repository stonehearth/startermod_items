#include <libjson.h>
#include "lua/pch.h"
#include "open.h"
#include "om/all_objects.h"
#include "om/all_components.h"
#include "om/object_formatter/object_formatter.h"
#include "lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;
using namespace luabind;

luabind::object lua_to_json(dm::Store const& store, lua_State* L, JSONNode const& node)
{
   try {
      if (node.type() == JSON_STRING) {
         dm::ObjectPtr obj = om::ObjectFormatter().GetObject(store, node.as_string());
         if (obj) {

#define OM_OBJECT(Cls, lower) \
            case Cls ## ObjectType: return object(L, std::weak_ptr<Cls>(std::static_pointer_cast<Cls>(obj)));

            switch(obj->GetObjectType()) {
               OM_ALL_COMPONENTS
               OM_ALL_OBJECTS
            }

#undef OM_OBJECT

            LOG(WARNING) << "unrecognized object type " << obj->GetObjectType() << " in lua_to_json!";
         }
      }
   } catch (std::exception& e) {
      LOG(WARNING) << "critical error in lua_to_json: " << e.what();
   }
   return luabind::object();
}

void lua::om::open(lua_State* L)
{
}

void lua::om::register_json_to_lua_objects(lua_State* L, dm::Store& dm)
{
   lua::ScriptHost* sh = ScriptHost::GetScriptHost(L);
   sh->AddJsonToLuaConverter([&dm](lua_State* L, JSONNode const& node) {
      return lua_to_json(dm, L, node);
   });
}
