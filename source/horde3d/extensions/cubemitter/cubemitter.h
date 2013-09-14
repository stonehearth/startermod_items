#ifndef _RADIANT_HORDE3D_EXTENSIONS_CUBEMITTER_H
#define _RADIANT_HORDE3D_EXTENSIONS_CUBEMITTER_H

#include "datachannel.h"
#include "egPrerequisites.h"
#include "egScene.h"
#include "csg/region.h"
#include "csg/color.h"
#include "namespace.h"
//#include "radiant.pb.h"
#include "../extension.h"

using namespace ::Horde3D;

BEGIN_RADIANT_HORDE3D_NAMESPACE

class CubemitterResource;
typedef SmartResPtr< CubemitterResource > PCubemitterResource;

struct CubemitterNodeTpl : public SceneNodeTpl
{
   PCubemitterResource _cubemitterRes;
   PMaterialResource _matRes;

   CubemitterNodeTpl(const std::string &name, const PCubemitterResource &cubeRes, const PMaterialResource &matRes) :
      SceneNodeTpl(SNT_CubemitterNode, name),
      _cubemitterRes(cubeRes), _matRes(matRes)
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


struct EmissionData 
{
   DataChannel<float> *rate;
};

struct ColorData 
{
   DataChannel<Vec3f> *start;
   DataChannel<float> *over_lifetime_r;
   DataChannel<float> *over_lifetime_g;
   DataChannel<float> *over_lifetime_b;
};

struct ScaleData 
{
   DataChannel<float> *start;
   DataChannel<float> *over_lifetime;
};

struct LifetimeData 
{
   DataChannel<float> *start;
};

struct SpeedData
{
   DataChannel<float> *start;
};

struct ParticleData 
{
   LifetimeData lifetime;
   SpeedData speed;
   ColorData color;
   ScaleData scale;
};


struct CubemitterData {
   float duration;
   EmissionData emission;
   ParticleData particle;
};

struct CubeData
{
   float currentLife, maxLife;
   Vec3f position;
   float startScale, scale;
   Vec3f direction;
   Vec4f color;
   float speed;
};

struct CubeAttribute
{
   Matrix4f matrix;
   Vec4f color;
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
   CubemitterData emitterData;

private:
	bool raiseError( const std::string &msg, int line = -1 );
   EmissionData parseEmission(JSONNode& n);
   ParticleData parseParticle(JSONNode& n);
   ColorData parseColor(JSONNode& n);
   ScaleData parseScale(JSONNode& n);
   LifetimeData parseLifetime(JSONNode& n);
   SpeedData parseSpeed(JSONNode& n);

   
private:

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
   void updateAndSpawnCubes(int numToSpawn);
   void spawnCube(CubeData &d, CubeAttribute &ca);
   void updateCube(CubeData &d, CubeAttribute &ca);

protected:

   uint32                   _attributeBuf;
   PCubemitterResource      _cubemitterRes;
   uint32                   _maxCubes;
   float                    _nextSpawnTime;
   float                    _curEmitterTime;  // Bounded between 0 and the duration of the emitter.

   CubeAttribute            *_attributesBuff;






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

	std::vector< uint32 >    _occQueries;
	std::vector< uint32 >    _lastVisited;

	friend class SceneManager;
	friend class Renderer;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
