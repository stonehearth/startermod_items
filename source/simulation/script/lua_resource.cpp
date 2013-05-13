#include "pch.h"
#include "lua_resource.h"
#include "resources/animation.h"
#include "resources/data_resource.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace ::luabind;


void LuaResource::RegisterType(lua_State* L)
{
   module(L) [
      class_<resources::Resource>("Resource"),
      class_<resources::Animation, resources::Resource, std::shared_ptr<resources::Resource>>("AnimationResource")
         .def(tostring(self))
         .def("get_duration", &resources::Animation::GetDuration),
      class_<resources::DataResource, resources::Resource, std::shared_ptr<resources::Resource>>("DataResource")
         .def(tostring(self))
         .def("get_json", &resources::DataResource::GetJsonString)
   ];
}

luabind::object LuaResource::ConvertResourceToLua(lua_State *L, std::shared_ptr<resources::Resource> obj)
{
   luabind::object result;

   if (obj) {
      switch (obj->GetType()) {
      case resources::Resource::ANIMATION:
         return luabind::object(L, static_cast<resources::Animation*>(obj.get()));
      case resources::Resource::JSON:
         return luabind::object(L, static_cast<resources::DataResource*>(obj.get()));
      default:
         ASSERT(false);
         break;
      }
   }
   return result;
}