#include "pch.h"
#include "lua_renderer.h"

using namespace luabind;
using namespace ::radiant;
using namespace ::radiant::client;

static object GetNodeParam(lua_State* L, H3DNode node, int param)
{
   switch (param) {
   case H3DNodeParams::NameStr:
      return object(L, h3dGetNodeParamStr(node, param));
   }
   return object();
}

void LuaRenderer::RegisterType(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("renderer") [
            def("get_node_param",           &GetNodeParam),
            def("remove_node",              &h3dRemoveNode),
            def("create_designation_node",  &h3dRadiantCreateStockpileNode),
            def("resize_designation_node",  &h3dRadiantResizeStockpileNode)
         ]
      ]
   ];
};

