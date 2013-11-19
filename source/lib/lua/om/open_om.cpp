#include <libjson.h>
#include "lib/lua/pch.h"
#include "open.h"
#include "om/all_objects.h"
#include "om/all_components.h"
#include "om/object_formatter/object_formatter.h"
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::om;
using namespace luabind;

luabind::object json_to_lua(dm::Store const& store, lua_State* L, JSONNode const& node)
{
   try {
      if (node.type() == JSON_STRING) {
         dm::ObjectPtr obj = om::ObjectFormatter().GetObject(store, node.as_string());
         if (obj) {

#define OM_OBJECT(Cls, lower) \
            case Cls ## ObjectType: \
               return object(L, std::weak_ptr<Cls>(std::static_pointer_cast<Cls>(obj)));

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

template <typename T, typename Cls, T const& (Cls::*Fn)() const>
T PropertyGet(std::weak_ptr<Cls> const w)
{
   auto p = w.lock();
   if (p)  {
      return ((*p).*Fn)();
   }
   throw std::invalid_argument("invalid reference in native getter");
   return T(); // unreached
}

template <typename T, typename Cls, Cls& (Cls::*Fn)(T const&)>
std::weak_ptr<Cls> PropertySet(std::weak_ptr<Cls> w, T const& value)
{
   auto p = w.lock();
   if (p) {
      (((*p).*Fn)(value));
   }
   throw std::invalid_argument("invalid reference in native setter");
   return w;
}

void lua::om::open(lua_State* L)
{
   using namespace radiant::om;

   #include "om/reflection/generate_lua.h"
   OM_ALL_COMPONENT_TEMPLATES
}

void lua::om::register_json_to_lua_objects(lua_State* L, dm::Store& dm)
{
   lua::ScriptHost* sh = ScriptHost::GetScriptHost(L);
   sh->AddJsonToLuaConverter([&dm](lua_State* L, JSONNode const& node) {
      return json_to_lua(dm, L, node);
   });
}
