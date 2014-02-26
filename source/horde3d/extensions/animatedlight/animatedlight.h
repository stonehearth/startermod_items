#ifndef _RADIANT_HORDE3D_EXTENSIONS_ANIMATEDLIGHT_H
#define _RADIANT_HORDE3D_EXTENSIONS_ANIMATEDLIGHT_H

#include "egPrerequisites.h"
#include "egScene.h"
#include "csg/region.h"
#include "csg/color.h"
#include "datachannel.h"
#include "namespace.h"
#include "../extension.h"

using namespace ::Horde3D;
using namespace ::radiant::json;

BEGIN_RADIANT_HORDE3D_NAMESPACE

class AnimatedLightResource;
typedef SmartResPtr< AnimatedLightResource > PAnimatedLightResource;

struct AnimatedLightNodeTpl : public SceneNodeTpl
{
   PAnimatedLightResource _animatedLightRes;

   AnimatedLightNodeTpl(const std::string &name, const PAnimatedLightResource &lightRes) :
      SceneNodeTpl(SNT_AnimatedLightNode, name), _animatedLightRes(lightRes)
   {
   }
};

namespace animatedlight
{
class ColorData 
{
public:
   ValueEmitter<Vec3f> *start;
   ValueEmitter<float> *over_lifetime_r;
   ValueEmitter<float> *over_lifetime_g;
   ValueEmitter<float> *over_lifetime_b;

   ColorData()
   {
      start = new ConstantValueEmitter<Vec3f>(Vec3f(1.0f, 0.0f, 0.0f));
      over_lifetime_r = new ConstantValueEmitter<float>(1.0f);
      over_lifetime_g = new ConstantValueEmitter<float>(0.0f);
      over_lifetime_b = new ConstantValueEmitter<float>(0.0f);
   }
};

class RadiusData
{
public:
   ValueEmitter<float>* start;
   ValueEmitter<float>* over_lifetime;
};

class IntensityData
{
public:
   ValueEmitter<float>* start;
   ValueEmitter<float>* over_lifetime;
};
}

class AnimatedLightData 
{
public:
   animatedlight::IntensityData intensity;
   animatedlight::ColorData color;
   animatedlight::RadiusData radius;
   float duration;
   bool loops;
};

// =================================================================================================

class AnimatedLightResource : public Resource
{
public:
   static Resource *factoryFunc( const std::string &name, int flags )
      { return new AnimatedLightResource( name, flags ); }

   AnimatedLightResource( const std::string &name, int flags );
   ~AnimatedLightResource();
   void initDefault();
   void release();
   bool load( const char *data, int size );

   int getElemCount( int elem );
   float getElemParamF( int elem, int elemIdx, int param, int compIdx );
   void setElemParamF( int elem, int elemIdx, int param, int compIdx, float value );
   AnimatedLightData lightData;

private:
   bool raiseError( const std::string &msg, int line = -1 );   
};

typedef SmartResPtr< AnimatedLightResource > PAnimatedLightResource;

// =================================================================================================


class AnimatedLightNode : public SceneNode
{
public:
   static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
   static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

   ~AnimatedLightNode();

   void init();
   int getParamI( int param );
   void setParamI( int param, int value );
   float getParamF( int param, int compIdx );
   void setParamF( int param, int compIdx, float value );

   void advanceTime( float timeDelta );
   bool hasFinished();

protected:
   AnimatedLightNode( const AnimatedLightNodeTpl &animatedLightTpl );

   void onPostUpdate();
   void updateLight();

   NodeHandle               _lightNode;
   PAnimatedLightResource   _animatedLightRes;
   bool                     _active;

   float                    _timeDelta;
   float                    _lightTime;
   std::vector< uint32 >    _occQueries;
   std::vector< uint32 >    _lastVisited;

   friend class SceneManager;
   friend class Renderer;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
