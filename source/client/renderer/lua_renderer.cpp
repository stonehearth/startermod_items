#include "pch.h"
#include "renderer.h"
#include "pipeline.h"
#include "client/client.h"
#include "lua_renderer.h"
#include "csg/point.h"
#include "h3d_resource_types.h"
#include "lib/json/core_json.h"
#include "lib/lua/register.h"
#include "raycast_result.h"
#include "Horde3DRadiant.h"

using namespace luabind;
using namespace ::radiant;
using namespace ::radiant::client;

void H3DNodeUnique_Destroy(H3DNodeUnique &node)
{
   node.reset(0);
}

static object GetNodeParam(lua_State* L, H3DNode node, int param)
{
   switch (param) {
   case H3DNodeParams::NameStr:
      return object(L, h3dGetNodeParamStr(node, param));
   }
   return object();
}

static void Camera_Translate(const csg::Point3f& delta) 
{
   Camera *c = Renderer::GetInstance().GetCamera();

   csg::Point3f curPos = c->GetPosition();
   c->SetPosition(curPos + delta);
}

static csg::Point3f Camera_GetForward() 
{
   Camera *c = Renderer::GetInstance().GetCamera();

   csg::Point3f forward, up, left;
   c->GetBases(&forward, &up, &left);

   return forward;
}

static csg::Point3f Camera_GetLeft() 
{
   Camera *c = Renderer::GetInstance().GetCamera();

   csg::Point3f forward, up, left;
   c->GetBases(&forward, &up, &left);

   return left;
}

static csg::Point3f Camera_GetPosition()
{
   return Renderer::GetInstance().GetCamera()->GetPosition();
}

static void Camera_SetPosition(const csg::Point3f& newPosition)
{
   Renderer::GetInstance().GetCamera()->SetPosition(newPosition);
}

static void Camera_LookAt(const csg::Point3f& target)
{
   Renderer::GetInstance().GetCamera()->LookAt(target);
}

static void Camera_SetOrientation(csg::Quaternion& angles)
{
   Renderer::GetInstance().GetCamera()->SetOrientation(angles);
}

static csg::Quaternion Camera_GetOrientation()
{
   return Renderer::GetInstance().GetCamera()->GetOrientation();
}

static csg::Point2f Camera_WorldToScreen(csg::Point3f const& pt)
{
   return Renderer::GetInstance().GetCamera()->WorldToScreen(pt);
}

static csg::Ray3 Scene_GetScreenRay(double windowX, double windowY)
{
   csg::Ray3 result;
   Renderer::GetInstance().GetCameraToViewportRay((int)windowX, (int)windowY, &result);
   return result;
}

static RaycastResult Scene_CastRay(const csg::Point3f& origin, const csg::Point3f& direction) 
{
   return Renderer::GetInstance().QuerySceneRay(origin, direction, 0);
}

static RaycastResult Scene_CastScreenRay(double windowX, double windowY) 
{
   return Renderer::GetInstance().QuerySceneRay((int)windowX, (int)windowY, 0);
}

static int Screen_GetWidth() {
   return Renderer::GetInstance().GetWindowWidth();
}

static int Screen_GetHeight() {
   return Renderer::GetInstance().GetWindowHeight();
}

static void Sky_SetSkyColor(const csg::Point3f& startCol, const csg::Point3f& endCol) {
   Renderer::GetInstance().SetSkyColors(startCol, endCol);
}

static void Sky_SetStarfieldBrightness(float brightness) {
   Renderer::GetInstance().SetStarfieldBrightness(brightness);
}

static void Renderer_SetVisibleRegion(std::string const& uri) {
   Renderer::GetInstance().SetVisibleRegion(uri);
}

static void Renderer_SetExploredRegion(std::string const& uri) {
   Renderer::GetInstance().SetExploredRegion(uri);
}

static void Renderer_EnablePerfLogging(bool enable)
{
   h3dSetOption(H3DOptions::EnableStatsLogging, enable ? 1.0f : 0.0f);
}

std::ostream& operator<<(std::ostream& os, RaycastResult const& r)
{
   os << "[RaycastResult of " << r.GetNumResults() << " results]";
   return os;
}

std::ostream& operator<<(std::ostream& os, RaycastResult::Result const& r)
{
   os << "[intersection:" << r.intersection << " normal:" << r.normal
      << " brick: " << r.brick << " entity:" << *r.entity.lock() << "]";
   return os;
}

void h3dSetGlobalUniformFloat(const char* name, float value)
{
   h3dSetGlobalUniform(name, H3DUniformType::FLOAT, &value);
}

DEFINE_INVALID_JSON_CONVERSION(RaycastResult);
DEFINE_INVALID_JSON_CONVERSION(RenderNode);
IMPLEMENT_TRIVIAL_TOSTRING(RenderNode);

void LuaRenderer::RegisterType(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("renderer") [
            def("enable_perf_logging", &Renderer_EnablePerfLogging),
            namespace_("camera") [
               def("translate",    &Camera_Translate),
               def("get_forward",  &Camera_GetForward),
               def("get_left",     &Camera_GetLeft),
               def("get_position", &Camera_GetPosition),
               def("set_position", &Camera_SetPosition),
               def("look_at",      &Camera_LookAt),
               def("set_orientation", &Camera_SetOrientation),
               def("get_orientation", &Camera_GetOrientation),
               def("world_to_screen", &Camera_WorldToScreen)
            ],
            namespace_("scene") [
               lua::RegisterType_NoTypeInfo<RaycastResult::Result>("RaycastResultEntry")
                  .def_readwrite("intersection", &RaycastResult::Result::intersection)
                  .def_readwrite("normal", &RaycastResult::Result::normal)
                  .def_readwrite("brick", &RaycastResult::Result::brick)
                  .def_readwrite("entity", &RaycastResult::Result::entity)
               ,
               lua::RegisterType_NoTypeInfo<RaycastResult>("RaycastResult")
                  .def("get_result_count",       &RaycastResult::GetNumResults)
                  .def("each_result",       &RaycastResult::GetResults, return_stl_iterator)
                  .def("get_ray",           &RaycastResult::GetRay)
                  .def("get_result",        &RaycastResult::GetResult)
               ,
               def("cast_screen_ray",    &Scene_CastScreenRay),
               def("cast_ray",           &Scene_CastRay),
               def("get_screen_ray",     &Scene_GetScreenRay)
            ],
            namespace_("screen") [
               def("get_width",   &Screen_GetWidth),
               def("get_height",  &Screen_GetHeight)
            ],
            namespace_("sky") [
               def("set_sky_colors", &Sky_SetSkyColor),
               def("set_starfield_brightness", &Sky_SetStarfieldBrightness)
            ],
            namespace_("visibility") [
               def("set_visible_region",  &Renderer_SetVisibleRegion),
               def("set_explored_region", &Renderer_SetExploredRegion)
            ]
         ]
      ],

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
		   value("Pipeline",                   H3DResTypes::Pipeline),
         value("Cubemitter",                 RT_CubemitterResource)
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
      class_<H3DNodeParams>("H3DNodeParams")
         .enum_("constants")
      [
         value("NameStr",                   H3DNodeParams::NameStr)
      ],
      class_<H3DNodeFlags>("H3DNodeFlags")
         .enum_("constants")
      [
         value("Inactive",                   H3DNodeFlags::Inactive),
         value("NoCastShadow",               H3DNodeFlags::NoCastShadow)
      ],
      class_<H3DProjectorNodeParams>("H3DProjectorNodeParams")
         .enum_("constants")
      [
         value("MatResI",                    H3DProjectorNodeParams::MatResI)
      ],
      class_<H3DNodeUnique>("H3DNodeUnique")
         .def("destroy",                     H3DNodeUnique_Destroy),
      lua::RegisterTypePtr_NoTypeInfo<RenderNode>("RenderNode")
         .def("get_node",                    &RenderNode::GetNode)
         .def("set_transform",               &RenderNode::SetTransform)
         .def("set_position",                &RenderNode::SetPosition)
         .def("set_rotation",                &RenderNode::SetRotation)
         .def("set_scale",                   &RenderNode::SetScale)
         .def("set_name",                    &RenderNode::SetName)
         .def("set_visible",                 &RenderNode::SetVisible)
         .def("set_can_query",               &RenderNode::SetCanQuery)
         .def("set_material",                static_cast<RenderNodePtr (RenderNode::*)(std::string const&)>(&RenderNode::SetMaterial))
         .def("destroy",                     &RenderNode::Destroy)
      ,
      def("h3dGetNodeParamStr",              &h3dGetNodeParamStr),
      def("h3dRemoveNode",                   &h3dRemoveNode),
      def("h3dAddProjectorNode",             &h3dAddProjectorNode),
      def("h3dAddLightNode",                 &h3dAddLightNode),
      def("h3dRadiantAddCubemitterNode",     &h3dRadiantAddCubemitterNode),
      def("h3dAddResource",                  &h3dAddResource),
      def("h3dAddGroupNode",                 &h3dAddGroupNode),
      def("h3dSetMaterialUniform",           &h3dSetMaterialUniform),
      def("h3dSetNodeTransform",             &h3dSetNodeTransform),
      def("h3dSetNodeParamI",                &h3dSetNodeParamI),
      def("h3dSetNodeParamF",                &h3dSetNodeParamF),
      def("h3dSetNodeParamStr",              &h3dSetNodeParamStr),
      def("h3dSetNodeFlags",                 &h3dSetNodeFlags),
      def("h3dGetNodeFlags",                 &h3dGetNodeFlags),
      def("h3dSetGlobalUniform",             &h3dSetGlobalUniformFloat)
   ];
   globals(L)["H3DRootNode"] = H3DRootNode;
};
