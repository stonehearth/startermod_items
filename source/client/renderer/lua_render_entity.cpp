#include "pch.h"
#include "renderer.h"
#include "lua_render_entity.h"

using namespace ::radiant;
using namespace ::radiant::client;

volatile int i = -1;
void f()
{
   i++;
}

Renderer& RenderEntity_GetRenderer(RenderEntity const& e)
{
   return Renderer::GetInstance();
}

om::EntityRef RenderEntity_GetEntity(RenderEntity const& e)
{
   return e.GetEntity();
}

static std::ostream& operator<<(std::ostream& os, const RenderEntity& o)
{
   return (os << "[RenderEntity " << o.GetObjectId() << "]");
}

void LuaRenderEntity::RegisterType(lua_State* L)
{
   using namespace luabind;

   atexit(f);
   module(L) [
      namespace_("_radiant") [
         namespace_("renderer") [
            class_<RenderEntity, std::weak_ptr<RenderEntity>>("RenderEntity")
               .def(tostring(self))
               .def("get_node",        &RenderEntity::GetNode)
               .def("get_entity",      &RenderEntity_GetEntity)
               .def("get_renderer",    &RenderEntity_GetRenderer)
         ]
      ]
   ];
};
