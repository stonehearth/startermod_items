#include "lib/lua/pch.h"
#include "open.h"
#include "resources/res_manager.h"
#include "resources/animation.h"
#include "lib/lua/script_host.h"
#include "lib/json/core_json.h"

using namespace ::radiant;
using namespace ::radiant::res;
using namespace luabind;

AnimationPtr load_animation(std::string const& uri)
{
   return ResourceManager2::GetInstance().LookupAnimation(uri);
}

object load_json(lua_State* L, std::string const& uri)
{
   object result;

   res::ResourceManager2::GetInstance().LookupJson(uri, [&](const json::Node& node) {
      result = lua::ScriptHost::JsonToLua(L, node);
   });

   return result;
}

object load_manifest(lua_State* L, std::string const& mod_name)
{
   object result;

   res::ResourceManager2::GetInstance().LookupManifest(mod_name, [&](const res::Manifest& m) {
      result = lua::ScriptHost::JsonToLua(L, m.get_internal_node());
   });

   return result;
}

object ConvertToCanonicalPath(lua_State* L, std::string const& path)
{
   object result;
   try {
      result = object(L, res::ResourceManager2::GetInstance().ConvertToCanonicalPath(path, ".json"));
   } catch (std::exception const&) {
      // return LUA_TNIL...
   }
   return result;
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
            def("convert_to_canonical_path", &ConvertToCanonicalPath),

            lua::RegisterTypePtr_NoTypeInfo<Animation>("Animation")
               .def("get_duration",    &Animation::GetDuration)
         ]
      ]
   ];
}
