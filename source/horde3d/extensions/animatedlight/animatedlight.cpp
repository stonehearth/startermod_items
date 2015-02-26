#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "egCamera.h"
#include "libjson.h"
#include "lib/json/node.h"
#include "Horde3D.h"

#if defined(ASSERT)
#  undef ASSERT
#endif

#include "extension.h"
#include "datachannel.h"
#include "radiant.h"
#include "animatedlight.h"

using namespace ::radiant;
using namespace ::radiant::json;
using namespace ::radiant::horde3d;


animatedlight::ColorData parseColor(Node& n)
{
   animatedlight::ColorData result;
   result.start = parseChannel(n, "start", Vec3f(1, 0, 0));
   result.over_lifetime_r = parseChannel(n, "over_lifetime_r", result.start->nextValue(0).x);
   result.over_lifetime_g = parseChannel(n, "over_lifetime_g", result.start->nextValue(0).y);
   result.over_lifetime_b = parseChannel(n, "over_lifetime_b", result.start->nextValue(0).z);
   return result;
}

animatedlight::IntensityData parseIntensity(Node& n)
{
   animatedlight::IntensityData result;
   result.start = parseChannel(n, "start", 1.0f);
   result.over_lifetime = parseChannel(n, "over_lifetime", result.start->nextValue(0));
   return result;
}

animatedlight::RadiusData parseRadius(Node& n, float def)
{
   animatedlight::RadiusData result;
   result.start = parseChannel(n, "start", def);
   result.over_lifetime = parseChannel(n, "over_lifetime", result.start->nextValue(0));
   return result;
}

// *************************************************************************************************
// AnimatedLightResource
// *************************************************************************************************

AnimatedLightResource::AnimatedLightResource(std::string const& name, int flags) :
   Resource(RT_AnimatedLightResource, name, flags)
{
   initDefault();	
}

AnimatedLightResource::~AnimatedLightResource()
{
   release();
}

void AnimatedLightResource::initDefault()
{
}

void AnimatedLightResource::release()
{
}

bool AnimatedLightResource::raiseError(std::string const& msg, int line)
{
   // Reset
   release();
   initDefault();

   if (line < 0) {
      Modules::log().writeError("AnimatedLight resource '%s': %s", _name.c_str(), msg.c_str());
   } else {
      Modules::log().writeError("AnimatedLight resource '%s' in line %i: %s", _name.c_str(), line, msg.c_str());
   }

   return false;
}

bool AnimatedLightResource::load(const char *data, int size)
{
   if (!Resource::load(data, size)) {
      return false;
   }

   std::string jsonData(data, size);
   Node root(libjson::parse(jsonData));

   lightData.intensity = parseIntensity(root.get_node("intensity"));

   lightData.color = parseColor(root.get_node("color"));

   lightData.radius = parseRadius(root.get_node("radius"), 20.0);

   lightData.inner_radius = parseRadius(root.get_node("inner_radius"), 0.0);

   lightData.importance = root.get("importance", (int)H3DLightImportance::High);

   lightData.loops = root.get("loops", true);

   lightData.duration = root.get("duration", 5.0f);

   return true;
}

int AnimatedLightResource::getElemCount(int elem)
{
   return Resource::getElemCount(elem);
}

float AnimatedLightResource::getElemParamF(int elem, int elemIdx, int param, int compIdx)
{
   return Resource::getElemParamF(elem, elemIdx, param, compIdx);
}

void AnimatedLightResource::setElemParamF(int elem, int elemIdx, int param, int compIdx, float value)
{
   Resource::setElemParamF(elem, elemIdx, param, compIdx, value);
}

// *************************************************************************************************
// AnimatedLightNode
// *************************************************************************************************


AnimatedLightNode::AnimatedLightNode(const AnimatedLightNodeTpl &animatedLightTpl) :
   SceneNode(animatedLightTpl)
{
   _renderable = false;
   _animatedLightRes = animatedLightTpl._animatedLightRes;
   _timeDelta = 0.0f;
   _lightTime = 0.0f;
   _active = true;
}

void AnimatedLightNode::init()
{
   // Create with out any shadows; the update loop will set the appropriate contexts.  That way, we
   // can turn shadows on/off at run-time.
   _lightNode = h3dAddLightNode(this->getHandle(), "ln", "OMNI_LIGHTING_NO_SHADOW", "", false);
   h3dSetNodeParamF(_lightNode, H3DLight::FovF, 0, 360);
   h3dSetNodeParamI(_lightNode, H3DLight::ShadowMapCountI, 0);
   h3dSetNodeParamF(_lightNode, H3DLight::ColorF3, 0, 0.0f);
   h3dSetNodeParamF(_lightNode, H3DLight::ColorF3, 1, 0.0f);
   h3dSetNodeParamF(_lightNode, H3DLight::ColorF3, 2, 0.0f);
   h3dSetNodeParamF(_lightNode, H3DLight::Radius1F, 0, 0.0f);
   h3dSetNodeParamF(_lightNode, H3DLight::Radius2F, 0, 0.0f);
}

AnimatedLightNode::~AnimatedLightNode()
{
   h3dRemoveNode(_lightNode);
   _lightNode = 0x0;
   _animatedLightRes = 0x0;
   _active = false;
}


SceneNodeTpl *AnimatedLightNode::parsingFunc(std::map< std::string, std::string > &attribs)
{
   return nullptr;
}


SceneNode *AnimatedLightNode::factoryFunc(const SceneNodeTpl &nodeTpl)
{
   if (nodeTpl.type != SNT_AnimatedLightNode) {
      return 0x0;
   }

   return new AnimatedLightNode(*(AnimatedLightNodeTpl *)&nodeTpl);
}


int AnimatedLightNode::getParamI(int param) const
{
   return SceneNode::getParamI(param);
}


void AnimatedLightNode::setParamI(int param, int value)
{
   switch(param) {
   case H3DLight::ShadowMapCountI:
      h3dSetNodeParamI(_lightNode, H3DLight::ShadowMapCountI, value);
      return;
   }
   SceneNode::setParamI(param, value);
}


float AnimatedLightNode::getParamF(int param, int compIdx)
{
   return SceneNode::getParamF(param, compIdx);
}


void AnimatedLightNode::setParamF(int param, int compIdx, float value)
{
   SceneNode::setParamF(param, compIdx, value);
}


void AnimatedLightNode::advanceTime(float timeDelta)
{
   _timeDelta += timeDelta;
   _lightTime += timeDelta;

   markDirty(SceneNodeDirtyKind::Ancestors);
}


bool AnimatedLightNode::hasFinished()
{
   return !_active;
}

void AnimatedLightNode::onPostUpdate()
{	
   if( _timeDelta <= 0 || !_active /*|| _effectRes == 0x0*/ ) return;

   updateLight();   
}

void AnimatedLightNode::updateLight()
{
   AnimatedLightData d = _animatedLightRes.getPtr()->lightData;

   bool shadows = h3dGetNodeParamI(_lightNode, H3DLight::ShadowMapCountI) > 0;

   h3dSetNodeParamI(_lightNode, H3DLight::ImportanceI, d.importance);
   h3dSetNodeParamStr(_lightNode, H3DLight::LightingContextStr, shadows ? "OMNI_LIGHTING" : "OMNI_LIGHTING_NO_SHADOW");
   h3dSetNodeParamStr(_lightNode, H3DLight::ShadowContextStr, shadows ? "OMNI_SHADOWMAP" : "");

   if (_lightTime >= d.duration)
   {
      if (d.loops)
      {
         _lightTime -= d.duration;
         d.intensity.over_lifetime->init();
         d.color.over_lifetime_r->init();
         d.color.over_lifetime_g->init();
         d.color.over_lifetime_b->init();
      } else {
         _active = false;
         return;
      }
   }
   float t = _lightTime / d.duration;

   float intensity = d.intensity.over_lifetime->nextValue(t);
   h3dSetNodeParamF(_lightNode, H3DLight::ColorF3, 0, d.color.over_lifetime_r->nextValue(t) * intensity);
   h3dSetNodeParamF(_lightNode, H3DLight::ColorF3, 1, d.color.over_lifetime_g->nextValue(t) * intensity);
   h3dSetNodeParamF(_lightNode, H3DLight::ColorF3, 2, d.color.over_lifetime_b->nextValue(t) * intensity);
   h3dSetNodeParamF(_lightNode, H3DLight::Radius2F, 0, d.radius.over_lifetime->nextValue(t));
   h3dSetNodeParamF(_lightNode, H3DLight::Radius1F, 0, d.inner_radius.over_lifetime->nextValue(t));
}