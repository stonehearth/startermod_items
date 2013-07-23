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
      class_<H3DResTypes>("H3DResTypes")
         .enum_("constants")
      [
		   value("Undefined",      H3DResTypes::Undefined),
		   value("SceneGraph",     H3DResTypes::SceneGraph),
		   value("Geometry",       H3DResTypes::Geometry),
		   value("Animation",      H3DResTypes::Animation),
		   value("Material",       H3DResTypes::Material),
		   value("Code",           H3DResTypes::Code),
		   value("Shader",         H3DResTypes::Shader),
		   value("Texture",        H3DResTypes::Texture),
		   value("ParticleEffect", H3DResTypes::ParticleEffect),
		   value("Pipeline",       H3DResTypes::Pipeline)
      ],
      def("h3dGetNodeParamStr",             &h3dGetNodeParamStr),
      def("h3dRemoveNode",                  &h3dRemoveNode),
      def("h3dAddLightNode",                &h3dAddLightNode),
      def("h3dAddResource",                 &h3dAddResource),
      def("h3dRadiantCreateStockpileNode",  &h3dRadiantCreateStockpileNode),
      def("h3dRadiantResizeStockpileNode",  &h3dRadiantResizeStockpileNode)
   ];
   globals(L)["H3DRootNode"] = H3DRootNode;
};

