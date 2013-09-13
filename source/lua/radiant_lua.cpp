#include "pch.h"
#include "radiant_lua.h"

using namespace ::radiant;

luabind::object lua::JsonToLua(lua_State* L, JSONNode const& json)
{
   using namespace luabind;

   if (json.type() == JSON_NODE) {
      object table = newtable(L);
      for (auto const& entry : json) {
         table[entry.name()] = JsonToLua(L, entry);
      }
      return table;
   } else if (json.type() == JSON_ARRAY) {
      object table = newtable(L);
      for (unsigned int i = 0; i < json.size(); i++) {
         table[i + 1] = JsonToLua(L, json[i]);
      }
      return table;
   } else if (json.type() == JSON_STRING) {
      return object(L, json.as_string());
   } else if (json.type() == JSON_NUMBER) {
      return object(L, json.as_float());
   } else if (json.type() == JSON_BOOL) {
      return object(L, json.as_bool());
   }

   ASSERT(false);
   return object();
}
