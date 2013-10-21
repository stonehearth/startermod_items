#include "lua/pch.h"
#include "open.h"
#include "resources/res_manager.h"
#include "resources/animation.h"
#include "lua/script_host.h"
#include "lib/json/core_json.h"

using namespace ::radiant;
using namespace ::radiant::res;
using namespace luabind;

AnimationPtr load_animation(std::string uri)
{
   return ResourceManager2::GetInstance().LookupAnimation(uri);
}

object load_json(lua_State* L, std::string const& uri)
{
   JSONNode node = res::ResourceManager2::GetInstance().LookupJson(uri);
   return lua::ScriptHost::JsonToLua(L, node);
}

object load_manifest(lua_State* L, std::string const& mod_name)
{
   res::Manifest m = res::ResourceManager2::GetInstance().LookupManifest(mod_name);
   return lua::ScriptHost::JsonToLua(L, m.GetNode());
}

DEFINE_INVALID_JSON_CONVERSION(Animation);

void lua::res::open(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("res") [
            def("load_json",        &load_json),
            def("load_animation",   &load_animation),
            def("load_manifest",    &load_manifest),

            lua::RegisterTypePtr<Animation>()
               .def("get_duration",    &Animation::GetDuration)
         ]
      ]
   ];
}
