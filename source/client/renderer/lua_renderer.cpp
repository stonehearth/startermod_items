#include "pch.h"
#include "renderer.h"
#include "client/client.h"
#include "om/object_formatter/object_formatter.h"
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
		   value("Undefined",                  H3DResTypes::Undefined),
		   value("SceneGraph",                 H3DResTypes::SceneGraph),
		   value("Geometry",                   H3DResTypes::Geometry),
		   value("Animation",                  H3DResTypes::Animation),
		   value("Material",                   H3DResTypes::Material),
		   value("Code",                       H3DResTypes::Code),
		   value("Shader",                     H3DResTypes::Shader),
		   value("Texture",                    H3DResTypes::Texture),
		   value("ParticleEffect",             H3DResTypes::ParticleEffect),
		   value("Pipeline",                   H3DResTypes::Pipeline)
      ],
      class_<H3DLight>("H3DLight")
         .enum_("constants")
      [
         value("MatResI",                    H3DLight::MatResI),
		   value("RadiusF",                    H3DLight::RadiusF),
		   value("FovF",                       H3DLight::FovF),
		   value("ColorF3",                    H3DLight::ColorF3),
		   value("ColorMultiplierF",           H3DLight::ColorMultiplierF),
		   value("AmbientColorF3",             H3DLight::AmbientColorF3),
		   value("ShadowMapCountI",            H3DLight::ShadowMapCountI),
		   value("ShadowSplitLambdaF",         H3DLight::ShadowSplitLambdaF),
		   value("ShadowMapBiasF",             H3DLight::ShadowMapBiasF),
		   value("LightingContextStr",         H3DLight::LightingContextStr),
		   value("ShadowContextStr",           H3DLight::ShadowContextStr),
         value("DirectionalI",               H3DLight::DirectionalI)
      ],
      def("h3dGetNodeParamStr",              &h3dGetNodeParamStr),
      def("h3dRemoveNode",                   &h3dRemoveNode),
      def("h3dAddLightNode",                 &h3dAddLightNode),
      def("h3dAddResource",                  &h3dAddResource),
      def("h3dRadiantCreateStockpileNode",   &h3dRadiantCreateStockpileNode),
      def("h3dRadiantResizeStockpileNode",   &h3dRadiantResizeStockpileNode),
      def("h3dSetMaterialUniform",           &h3dSetMaterialUniform),
      def("h3dSetNodeTransform",             &h3dSetNodeTransform),
      def("h3dSetNodeParamI",                &h3dSetNodeParamI),
      def("h3dSetNodeParamF",                &h3dSetNodeParamF)
   ];
   globals(L)["H3DRootNode"] = H3DRootNode;
};
