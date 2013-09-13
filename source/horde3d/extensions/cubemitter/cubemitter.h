#ifndef _RADIANT_HORDE3D_EXTENSIONS_CUBEMITTER_H
#define _RADIANT_HORDE3D_EXTENSIONS_CUBEMITTER_H

#include "egPrerequisites.h"
#include "egScene.h"
#include "csg/region.h"
#include "csg/color.h"
#include "namespace.h"
//#include "radiant.pb.h"
#include "../extension.h"

using namespace ::Horde3D;

BEGIN_RADIANT_HORDE3D_NAMESPACE

struct CubemitterNodeTpl : public SceneNodeTpl
{
   uint32 maxParticleCount;
   PMaterialResource matRes;

   CubemitterNodeTpl(const std::string &name, uint32 maxParticleCount) :
      SceneNodeTpl(SNT_CubemitterNode, name),
      maxParticleCount(maxParticleCount)
   {
   }
};

struct CubemitterNodeParams
{
	enum List
	{
		MaxCountI
	};
};

struct DataChannel
{
   enum Kind
   {
      CONSTANT,
      RANDOM_BETWEEN
   };

   enum DataKind
   {
      SCALAR,
      TRIPLE
   };

   struct Triple {
      float v0, v1, v2;
   };

   struct ChannelValue {
      union {
         float scalar;
         Triple triple;
      } value;

      ChannelValue(float f) {
         value.scalar = f;
      }

      ChannelValue(float v0, float v1, float v2) {
         value.triple.v0 = v0;
         value.triple.v1 = v1;
         value.triple.v2 = v2;
      }
   };

   DataKind dataKind;
   Kind kind;
   std::vector<ChannelValue> values;
};

struct EmissionData {
   DataChannel rate;
};

struct ColorData {
   DataChannel start;
};

struct ParticleData {
   DataChannel start_lifetime;
   DataChannel start_speed;
   ColorData color;
};

struct CubemitterData {
   float duration;
   EmissionData emission;
   ParticleData particle;
};

struct CubeData
{
	float   life, maxLife;
	Vec3f   dir, dragVec;
	uint32  respawnCounter;

	// Start values
	float  moveVel0, rotVel0, drag0;
	float  size0;
	float  r0, g0, b0, a0;
};

// =================================================================================================

class CubemitterResource : public Resource
{
public:
	static Resource *factoryFunc( const std::string &name, int flags )
		{ return new CubemitterResource( name, flags ); }
	
	CubemitterResource( const std::string &name, int flags );
	~CubemitterResource();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );

	int getElemCount( int elem );
	float getElemParamF( int elem, int elemIdx, int param, int compIdx );
	void setElemParamF( int elem, int elemIdx, int param, int compIdx, float value );

private:
	bool raiseError( const std::string &msg, int line = -1 );
   EmissionData parseEmission(JSONNode& n);
   ParticleData parseParticle(JSONNode& n);
   ColorData parseColor(JSONNode& n);
   DataChannel parseDataChannel(JSONNode& n);
   std::vector<DataChannel::ChannelValue> parseChannelValues(JSONNode& n);
   DataChannel::Kind parseChannelKind(std::string& kindName);
   DataChannel::DataKind extractDataKind(std::vector<DataChannel::ChannelValue> &values);

private:
   CubemitterData emitterData;

	friend class EmitterNode;
};

typedef SmartResPtr< CubemitterResource > PCubemitterResource;

// =================================================================================================


class CubemitterNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );
   static void renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                   const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet);

	~CubemitterNode();

	int getParamI( int param );
	void setParamI( int param, int value );
	float getParamF( int param, int compIdx );
	void setParamF( int param, int compIdx, float value );

	void advanceTime( float timeDelta );
	bool hasFinished();

protected:
	CubemitterNode( const CubemitterNodeTpl &emitterTpl );
	void setMaxParticleCount( uint32 maxParticleCount );

   void renderInstancedFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                   const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet);

	void onPostUpdate();

protected:
	// Emitter data
	float                    _timeDelta;
	float                    _emissionAccum;
	Matrix4f                 _prevAbsTrans;
	
	// Emitter params
	PMaterialResource        _materialRes;
	//PParticleEffectResource  _effectRes;
	uint32                   _particleCount;
	int                      _respawnCount;
	float                    _delay, _emissionRate, _spreadAngle;
	Vec3f                    _force;

	// Particle data
	CubeData                 *_cubes;
	float                    *_parPositions;
	float                    *_parSizesANDRotations;
	float                    *_parColors;

   uint32                   _attributeBuf;

	std::vector< uint32 >    _occQueries;
	std::vector< uint32 >    _lastVisited;

	friend class SceneManager;
	friend class Renderer;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
