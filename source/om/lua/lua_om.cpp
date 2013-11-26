#include "pch.h"
#include "lua_component.h"
#include "lua_entity.h"
#include "lib/lua/script_host.h"

#include "lib/json/core_json.h"
#include "lib/json/dm_json.h"

#include "dm/store.h"
#include "om/all_object_defs.h"
#include "om/all_component_defs.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

DEFINE_INVALID_JSON_CONVERSION(om::Region3BoxedPtrBoxed)


#define OM_OBJECT(Cls, cls) scope Register ## Cls(lua_State* L);
OM_ALL_COMPONENTS
#undef OM_OBJECT
extern scope RegisterDataStore(lua_State* L);

scope RegisterLuaComponents(lua_State *L)
{
   return scope();
}

void radiant::om::RegisterLuaTypes(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("om") [
            LuaComponent_::RegisterLuaTypes(L),
#define OM_OBJECT(Cls, cls) Register ## Cls(L),
            OM_ALL_COMPONENTS
#undef OM_OBJECT
            LuaEntity::RegisterLuaTypes(L),
            RegisterDataStore(L)
         ]
      ]
   ];
}
