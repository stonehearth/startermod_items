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


struct OriginData
{
   enum SurfaceKind
   {
      POINT,
      RECTANGLE
   };
   SurfaceKind surfaceKind;

   float length, width;
};

struct EmissionData 
{
   ValueEmitter<float> *rate;
   ValueEmitter<float> *angle;
   OriginData origin;
};

struct ColorData 
{
   ValueEmitter<Vec4f> *start;
   ValueEmitter<float> *over_lifetime_r;
   ValueEmitter<float> *over_lifetime_g;
   ValueEmitter<float> *over_lifetime_b;
   ValueEmitter<float> *over_lifetime_a;
};

struct ScaleData 
{
   ValueEmitter<float> *start;
   ValueEmitter<float> *over_lifetime;
};

struct RotationData
{
   ValueEmitter<float> *over_lifetime_x;
   ValueEmitter<float> *over_lifetime_y;
   ValueEmitter<float> *over_lifetime_z;
};

struct LifetimeData 
{
   ValueEmitter<float> *start;
};

struct SpeedData
{
   ValueEmitter<float> *start;
   ValueEmitter<float> *over_lifetime;
};

struct ParticleData 
{
   LifetimeData lifetime;
   SpeedData speed;
   ColorData color;
   ScaleData scale;
   RotationData rotation;
};


struct CubemitterData {
   float duration;
   EmissionData emission;
   ParticleData particle;
};

// The data for an individual cube, as used within the particle system.
struct CubeData
{
   float currentLife, maxLife;
   float currentScale, startScale;
   float currentSpeed, startSpeed;
   Vec4f currentColor, startColor;

   Vec3f position;
   Vec3f direction;

   // Values that change over time, with respect to the individual particle.
   ValueEmitter<float> *color_r;
   ValueEmitter<float> *color_g;
   ValueEmitter<float> *color_b;
   ValueEmitter<float> *color_a;

   ValueEmitter<float> *rotation_x;
   ValueEmitter<float> *rotation_y;
   ValueEmitter<float> *rotation_z;

   ValueEmitter<float> *scale;
   ValueEmitter<float> *speed;
};

// Layout of data for VBOs.
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
   OriginData parseOrigin(JSONNode &n);
   RotationData parseRotation(JSONNode& n);

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
	PMaterialResource        _materialRes;
   uint32                   _maxCubes;
   float                    _nextSpawnTime;
   float                    _curEmitterTime, _emitterDuration;  // Bounded between 0 and the duration of the emitter.
	float                    _emissionRate, _spreadAngle;

   CubeAttribute            *_attributesBuff;






	// Emitter data
	float                    _timeDelta;
	float                    _emissionAccum;
	Matrix4f                 _prevAbsTrans;
	
	// Emitter params
	uint32                   _particleCount;
	int                      _respawnCount;
	Vec3f                    _force;

	// Particle data
	CubeData                 *_cubes;

	std::vector< uint32 >    _occQueries;
	std::vector< uint32 >    _lastVisited;

	friend class SceneManager;
	friend class Renderer;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
