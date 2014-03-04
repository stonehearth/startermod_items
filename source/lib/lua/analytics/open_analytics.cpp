#include "../pch.h"
#include "open.h"
#include "lib/analytics/design_event.h"
#include "lib/json/node.h"

#include "resources/res_manager.h"
#include "lib/json/core_json.h"


using namespace ::radiant;
using namespace ::radiant::analytics;
using namespace luabind;

IMPLEMENT_TRIVIAL_TOSTRING(DesignEvent)

DEFINE_INVALID_LUA_CONVERSION(DesignEvent)
DEFINE_INVALID_JSON_CONVERSION(DesignEvent)

void lua::analytics::open(lua_State *L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("analytics") [
            lua::RegisterType<DesignEvent>("DesignEvent")
               .def(constructor<std::string const&>())
               .def("set_value",    &DesignEvent::SetValue)
               .def("set_position", &DesignEvent::SetPosition)
               .def("set_area",     &DesignEvent::SetArea)
               .def("send_event",   &DesignEvent::SendEvent)
         ]
      ]
   ];
}