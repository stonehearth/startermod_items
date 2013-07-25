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

Renderer& RenderEntity_GetRenderer(std::weak_ptr<RenderEntity>)
{
   return Renderer::GetInstance();
}

void LuaRenderEntity::RegisterType(lua_State* L)
{
   using namespace luabind;

   atexit(f);
   module(L) [
      namespace_("_radiant") [
         namespace_("renderer") [
            class_<RenderEntity, std::weak_ptr<RenderEntity>>("RenderEntity")
               .def("get_node",        &RenderEntity::GetNode)
               .def("get_renderer",    &RenderEntity_GetRenderer)
         ]
      ]
   ];
};
