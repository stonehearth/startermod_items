#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "egMaterial.h"
#include "egCamera.h"
#include "libjson.h"
#include "radiant_json.h"

#if defined(ASSERT)
#  undef ASSERT
#endif

#include "extension.h"
#include "radiant.h"
#include "cubemitter.h"

using namespace ::radiant;
using namespace ::radiant::json;
using namespace ::radiant::horde3d;



// *************************************************************************************************
// CubemitterResource
// *************************************************************************************************

CubemitterResource::CubemitterResource( const std::string &name, int flags ) :
   Resource( RT_CubemitterResource, name, flags )
{
	initDefault();	
}

CubemitterResource::~CubemitterResource()
{
	release();
}

void CubemitterResource::initDefault()
{
}

void CubemitterResource::release()
{
}

bool CubemitterResource::raiseError( const std::string &msg, int line )
{
	// Reset
	release();
	initDefault();

	if( line < 0 )
		Modules::log().writeError( "Cubemitter resource '%s': %s", _name.c_str(), msg.c_str() );
	else
		Modules::log().writeError( "Cubemitter resource '%s' in line %i: %s", _name.c_str(), line, msg.c_str() );
	
	return false;
}

bool CubemitterResource::load( const char *data, int size )
{
	if( !Resource::load( data, size ) ) return false;

   std::string jsonData(data, size);
   ConstJsonObject root(libjson::parse(jsonData));

   emitterData.duration = root.get("duration", 10.0f);

   if (root.has("particle"))
   {
      emitterData.particle = parseParticle(root.getn("particle"));
   }
   if (root.has("emission"))
   {
      emitterData.emission = parseEmission(root.getn("emission"));
   }
	return true;
}

Vec4f parseVec4f(ConstJsonObject &vals, const Vec4f& def)
{
   return Vec4f(vals.get(0, def.x), vals.get(1, def.y), vals.get(2, def.z), vals.get(3, def.w));
}

Vec3f parseVec3f(ConstJsonObject &vals, const Vec3f& def)
{
   return Vec3f(vals.get(0, def.x), vals.get(1, def.y), vals.get(2, def.z));
}

std::vector<std::pair<float, float> > parseCurveValues(ConstJsonObject &n) {
   std::vector<std::pair<float, float> > result;

   for (const auto &child : n)
   {
      result.push_back(std::pair<float, float>((float)child.at(0).as_float(), (float)child.at(1).as_float()));
   }

   return result;
}

ValueEmitter<float>* parseChannel(ConstJsonObject &n, const char *childName, float def)
{
   if (!n.has(childName))
   {
      return new ConstantValueEmitter<float>(def);
   }

   auto childNode = n.getn(childName);

   if (!childNode.has("values") || !childNode.has("kind"))
   {
      return new ConstantValueEmitter<float>(def);
   }
   auto vals = childNode.getn("values");
   std::string kind = childNode.get<std::string>("kind");

   if (kind == "CONSTANT")
   {
      return new ConstantValueEmitter<float>(vals.get(0, def));
   } else if (kind == "RANDOM_BETWEEN")
   {
      return new RandomBetweenValueEmitter(vals.get(0, def), vals.get(1, def));
   } else if (kind == "CURVE")
   {
      return new LinearCurveValueEmitter(parseCurveValues(vals));
   } //else kind == "RANDOM_BETWEEN_CURVES"

   return new RandomBetweenLinearCurvesValueEmitter(
      parseCurveValues(vals.getn(0)), parseCurveValues(vals.getn(1)));
}

ValueEmitter<Vec3f>* parseChannel(ConstJsonObject &n, const char *childName, const Vec3f &def)
{
   if (!n.has(childName))
   {
      return new ConstantValueEmitter<Vec3f>(def);
   }

   auto childNode = n.getn(childName);

   if (!childNode.has("values") || !childNode.has("kind"))
   {
      return new ConstantValueEmitter<Vec3f>(def);
   }
   auto vals = childNode.getn("values");
   std::string kind = childNode.get<std::string>("kind");

   if (kind == "CONSTANT")
   {
      Vec3f val(parseVec3f(vals, def));
      return new ConstantValueEmitter<Vec3f>(val);
   } //else kind == "RANDOM_BETWEEN"

   ConstJsonObject vec1node = vals.getn(0);
   ConstJsonObject vec2node = vals.getn(1);

   return new RandomBetweenVec3fEmitter(parseVec3f(vec1node, def), parseVec3f(vec2node, def));
}

ValueEmitter<Vec4f>* parseChannel(ConstJsonObject &n, const char *childName, const Vec4f &def)
{
   if (!n.has(childName))
   {
      return new ConstantValueEmitter<Vec4f>(def);
   }

   auto childNode = n.getn(childName);

   if (!childNode.has("values") || !childNode.has("kind"))
   {
      return new ConstantValueEmitter<Vec4f>(def);
   }
   auto vals = childNode.getn("values");
   std::string kind = childNode.get<std::string>("kind");

   if (kind == "CONSTANT")
   {
      return new ConstantValueEmitter<Vec4f>(parseVec4f(vals, def));
   } //else kind == "RANDOM_BETWEEN"

   return new RandomBetweenVec4fEmitter(parseVec4f(vals.getn(0), def), 
      parseVec4f(vals.getn(1), def));
}

OriginData::SurfaceKind parseSurfaceKind(ConstJsonObject &n)
{
   if (n.as<std::string>() == "POINT") {
      return OriginData::POINT;
   }
   return OriginData::RECTANGLE;
}

EmissionData CubemitterResource::parseEmission(ConstJsonObject& n) 
{
   EmissionData result;
   result.rate = parseChannel(n, "rate", 1.0f);
   result.angle = parseChannel(n, "angle", 25.0f);
   if (n.has("origin"))
   {
      result.origin = parseOrigin(n.getn("origin"));
   }
   return result;
}

OriginData CubemitterResource::parseOrigin(ConstJsonObject &n) {
   OriginData result;
   if (n.has("surface"))
   {
      result.surfaceKind = parseSurfaceKind(n.getn("surface"));
   }
   if (result.surfaceKind == OriginData::SurfaceKind::RECTANGLE) {
      if (n.has("values"))
      {
         result.length = n.getn("values").get<float>(0);
         result.width = n.getn("values").get<float>(1);
      }
   }
   return result;
}

ParticleData CubemitterResource::parseParticle(ConstJsonObject& n) 
{
   ParticleData result;
   if (n.has("speed"))
   {
      result.speed = parseSpeed(n.getn("speed"));
   }

   if (n.has("lifetime"))
   {
      result.lifetime = parseLifetime(n.getn("lifetime"));
   }

   if (n.has("color"))
   {
      result.color = parseColor(n.getn("color"));
   }

   if (n.has("scale"))
   {
      result.scale = parseScale(n.getn("scale"));
   }

   if (n.has("rotation"))
   {
      result.rotation = parseRotation(n.getn("rotation"));
   }

   if (n.has("velocity"))
   {
      result.velocity = parseVelocity(n.getn("velocity"));
   }
   return result;
}

LifetimeData CubemitterResource::parseLifetime(ConstJsonObject& n)
{
   LifetimeData result;
   result.start = parseChannel(n, "start", 5.0f);
   return result;
}

SpeedData CubemitterResource::parseSpeed(ConstJsonObject& n)
{
   SpeedData result;
   result.start = parseChannel(n, "start", 5.0f);
   result.over_lifetime = parseChannel(n, "over_lifetime", 1.0f);
   return result;
}

RotationData CubemitterResource::parseRotation(ConstJsonObject& n)
{
   RotationData result;
   result.over_lifetime_x = parseChannel(n, "over_lifetime_x", 0.0f);
   result.over_lifetime_y = parseChannel(n, "over_lifetime_y", 0.0f);
   result.over_lifetime_z = parseChannel(n, "over_lifetime_z", 0.0f);
   return result;
}

VelocityData CubemitterResource::parseVelocity(ConstJsonObject& n)
{
   VelocityData result;
   result.over_lifetime_x = parseChannel(n, "over_lifetime_x", 0.0f);
   result.over_lifetime_y = parseChannel(n, "over_lifetime_y", 0.0f);
   result.over_lifetime_z = parseChannel(n, "over_lifetime_z", 0.0f);
   return result;
}

ColorData CubemitterResource::parseColor(ConstJsonObject& n)
{
   ColorData result;
   result.start = parseChannel(n, "start", Vec4f(1, 0, 0, 1));
   result.over_lifetime_r = parseChannel(n, "over_lifetime_r", result.start->nextValue(0).x);
   result.over_lifetime_g = parseChannel(n, "over_lifetime_g", result.start->nextValue(0).y);
   result.over_lifetime_b = parseChannel(n, "over_lifetime_b", result.start->nextValue(0).z);
   result.over_lifetime_a = parseChannel(n, "over_lifetime_a", result.start->nextValue(0).w);
   return result;
}

ScaleData CubemitterResource::parseScale(ConstJsonObject& n)
{
   ScaleData result;
   result.start = parseChannel(n, "start", 1.0f);
   result.over_lifetime = parseChannel(n, "over_lifetime", 1.0f);
   return result;
}

int CubemitterResource::getElemCount( int elem )
{
	return Resource::getElemCount( elem );
}

float CubemitterResource::getElemParamF( int elem, int elemIdx, int param, int compIdx )
{
	return Resource::getElemParamF( elem, elemIdx, param, compIdx );
}

void CubemitterResource::setElemParamF( int elem, int elemIdx, int param, int compIdx, float value )
{
	Resource::setElemParamF( elem, elemIdx, param, compIdx, value );
}

// *************************************************************************************************
// CubemitterNode
// *************************************************************************************************


CubemitterNode::CubemitterNode( const CubemitterNodeTpl &emitterTpl ) :
	SceneNode( emitterTpl )
{
	_renderable = true;
   _materialRes = emitterTpl._matRes;
   _cubemitterRes = emitterTpl._cubemitterRes;
   _maxCubes = 100;
	_cubes = new CubeData[_maxCubes];
   _attributesBuff = new CubeAttribute[_maxCubes];
   _timeDelta = 0.0f;
   _nextSpawnTime = 0.0f;
   _curEmitterTime = 0.0f;

   _attributeBuf = gRDI->createVertexBuffer(sizeof(CubeAttribute) * _maxCubes, 0x0);

   for (unsigned int i = 0; i < _maxCubes; i++) {
      _attributesBuff[i].matrix.toIdentity();
      _cubes[i].currentLife = 0.0f;
      _cubes[i].color_a = nullptr;
      _cubes[i].color_r = nullptr;
      _cubes[i].color_g = nullptr;
      _cubes[i].color_b = nullptr;
      _cubes[i].scale = nullptr;
      _cubes[i].speed = nullptr;
      _cubes[i].rotation_x = nullptr;
      _cubes[i].rotation_y = nullptr;
      _cubes[i].rotation_z = nullptr;
      _cubes[i].velocity_x = nullptr;
      _cubes[i].velocity_y = nullptr;
      _cubes[i].velocity_z = nullptr;
   }
}


CubemitterNode::~CubemitterNode()
{
	for( uint32 i = 0; i < _occQueries.size(); ++i )
	{
		if( _occQueries[i] != 0 )
			gRDI->destroyQuery( _occQueries[i] );
	}
	
   delete[] _cubes;
   delete[] _attributesBuff;
}


SceneNodeTpl *CubemitterNode::parsingFunc( std::map< std::string, std::string > &attribs )
{
	bool result = true;
	
   return nullptr;
}


SceneNode *CubemitterNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SNT_CubemitterNode ) return 0x0;

	return new CubemitterNode( *(CubemitterNodeTpl *)&nodeTpl );
}


void CubemitterNode::setMaxParticleCount( radiant::uint32 maxParticleCount )
{
}


int CubemitterNode::getParamI( int param )
{
	return SceneNode::getParamI( param );
}


void CubemitterNode::setParamI( int param, int value )
{
	SceneNode::setParamI( param, value );
}


float CubemitterNode::getParamF( int param, int compIdx )
{
	return SceneNode::getParamF( param, compIdx );
}


void CubemitterNode::setParamF( int param, int compIdx, float value )
{
	SceneNode::setParamF( param, compIdx, value );
}


float randomF( float min, float max )
{
	return (rand() / (float)RAND_MAX) * (max - min) + min;
}


void CubemitterNode::advanceTime( float timeDelta )
{
   _curEmitterTime += timeDelta;
   if (_curEmitterTime > _emitterDuration) {
      _curEmitterTime -= _emitterDuration;
   }

   if (_wasVisible)
   {
	   _timeDelta += timeDelta;
   	markDirty();
   }
}


bool CubemitterNode::hasFinished()
{
	if( _respawnCount < 0 ) return false;

	for( uint32 i = 0; i < _particleCount; ++i )
	{	
      if( _cubes[i].currentLife > 0)
		{
			return false;
		}
	}
	
	return true;
}

void CubemitterNode::renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                                 const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet)
{
	if( frust1 == 0x0 || Modules::renderer().getCurCamera() == 0x0 ) return;
	if( debugView ) return;  // Don't render particles in debug view

	MaterialResource *curMatRes = 0x0;

	GPUTimer *timer = Modules::stats().getGPUTimer( EngineStats::ParticleGPUTime );
	if( Modules::config().gatherTimeStats ) timer->beginQuery( Modules::renderer().getFrameID() );

	// Bind cube geometry
   gRDI->setVertexBuffer( 0, Extension::getCubemitterCubeVBO(), 0, sizeof( CubeVert ) );
   gRDI->setIndexBuffer( Extension::getCubemitterCubeIBO(), IDXFMT_16 );

	// Loop through and find all Cubemitters.
	for( const auto &entry : Modules::sceneMan().getRenderableQueue() )
	{
      if( entry.type != SNT_CubemitterNode ) continue; 
		
		CubemitterNode *emitter = (CubemitterNode *)entry.node;

		/*if( emitter->_particleCount == 0 ) continue;
		if( !emitter->_materialRes->isOfClass( theClass ) ) continue;
		
		// Occlusion culling
		uint32 queryObj = 0;
		if( occSet >= 0 )
		{
			if( occSet > (int)emitter->_occQueries.size() - 1 )
			{
				emitter->_occQueries.resize( occSet + 1, 0 );
				emitter->_lastVisited.resize( occSet + 1, 0 );
			}
			if( emitter->_occQueries[occSet] == 0 )
			{
				queryObj = gRDI->createOcclusionQuery();
				emitter->_occQueries[occSet] = queryObj;
				emitter->_lastVisited[occSet] = 0;
			}
			else
			{
				if( emitter->_lastVisited[occSet] != Modules::renderer().getFrameID() )
				{
					emitter->_lastVisited[occSet] = Modules::renderer().getFrameID();
				
					// Check query result (viewer must be outside of bounding box)
					if( nearestDistToAABB( frust1->getOrigin(), emitter->getBBox().min,
					                       emitter->getBBox().max ) > 0 &&
						gRDI->getQueryResult( emitter->_occQueries[occSet] ) < 1 )
					{
						Modules::renderer().pushOccProxy( 0, emitter->getBBox().min,
							emitter->getBBox().max, emitter->_occQueries[occSet] );
                  _wasVisible = false;
						continue;
					}
					else
						queryObj = emitter->_occQueries[occSet];
				}
			}
		}*/
		
		// Set material
		if( curMatRes != emitter->_materialRes )
		{
			if( !Modules::renderer().setMaterial( emitter->_materialRes, shaderContext ) ) {
            continue;
         }
			curMatRes = emitter->_materialRes;
		}

		// Set vertex layout
      gRDI->setVertexLayout( Extension::getCubemitterCubeVL() );
		
		//if( queryObj )
		//	gRDI->beginQuery( queryObj );
		
		// Shader uniforms
		ShaderCombination *curShader = Modules::renderer().getCurShader();
		if( curShader->uni_nodeId >= 0 )
		{
			float id = (float)emitter->getHandle();
			gRDI->setShaderConst( curShader->uni_nodeId, CONST_FLOAT, &id );
		}

      gRDI->updateBufferData(emitter->_attributeBuf, 0, sizeof(CubeAttribute) * emitter->_maxCubes, emitter->_attributesBuff);
      gRDI->setVertexBuffer(1, emitter->_attributeBuf, 0, sizeof(CubeAttribute));
      gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, 36, 0, emitter->_maxCubes);
      
      emitter->_wasVisible = true;

		/*if( queryObj )
			gRDI->endQuery( queryObj );*/
	}

	timer->endQuery();


   // Draw occlusion proxies
	//if( occSet >= 0 )
	//	Modules::renderer().drawOccProxies( 0 );
	
	gRDI->setVertexLayout( 0 );
}

void CubemitterNode::onPostUpdate()
{	
   if( _timeDelta <= 0 || !_wasVisible /*|| _effectRes == 0x0*/ ) return;
	
	Timer *timer = Modules::stats().getTimer( EngineStats::ParticleSimTime );
	if( Modules::config().gatherTimeStats ) timer->setEnabled( true );
	
	Vec3f bBMin( Math::MaxFloat, Math::MaxFloat, Math::MaxFloat );
	Vec3f bBMax( -Math::MaxFloat, -Math::MaxFloat, -Math::MaxFloat );
	
	/*
   if( _delay <= 0 )
		_emissionAccum += _emissionRate * _timeDelta;
	else
		_delay -= _timeDelta;
   */

   CubemitterData d = _cubemitterRes.getPtr()->emitterData;

   float timeAvailableToSpawn = _timeDelta - _nextSpawnTime;
   _nextSpawnTime -= _timeDelta;
   int numberToSpawn = 0;
   while (timeAvailableToSpawn > 0) 
   {
      _nextSpawnTime = 1.0f / d.emission.rate->nextValue(_curEmitterTime);
      timeAvailableToSpawn -= _nextSpawnTime;
      numberToSpawn++;
   }
   
   updateAndSpawnCubes(numberToSpawn);
   
   _timeDelta = 0.0f;

   timer->setEnabled(false);
}

void CubemitterNode::updateAndSpawnCubes(int numToSpawn) 
{
   for (uint32 i = 0; i < _maxCubes; i++)
   {
      CubeData &d = _cubes[i];
      CubeAttribute &ca = _attributesBuff[i];

      if (d.currentLife <= 0 && numToSpawn > 0) 
      {
         spawnCube(d, ca);
         numToSpawn--;
      }

      updateCube(d, ca);
   }
}

void CubemitterNode::spawnCube(CubeData &d, CubeAttribute &ca)
{
   CubemitterData data = _cubemitterRes.getPtr()->emitterData;

   d.maxLife = data.particle.lifetime.start->nextValue(_curEmitterTime);
   d.currentLife = d.maxLife;

   Matrix4f m = _absTrans;
   d.position = m.getTrans();

   if (data.emission.origin.surfaceKind == OriginData::SurfaceKind::RECTANGLE)
   {
      float randWidth = randomF(0, data.emission.origin.width);
      float randLength = randomF(0, data.emission.origin.length);
      Vec3f v1(m.c[0][0], m.c[0][1], m.c[0][2]);
      Vec3f v2(m.c[1][0], m.c[1][1], m.c[1][2]);

      d.position += (v1 * randWidth) + (v2 * randLength);
   }

   float newAngle = data.emission.angle->nextValue(_curEmitterTime);

   float angle = degToRad( newAngle );
   m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0;
   m.rotate( randomF( -angle, angle ), randomF( -angle, angle ), randomF( -angle, angle ) );
   d.direction = (m * Vec3f( 0, 0, -1 )).normalized();

   auto startCol = data.particle.color.start->nextValue(_curEmitterTime);
   d.startColor = Vec4f(startCol.x, startCol.y, startCol.z, startCol.w);
   d.currentColor = d.startColor;

   // TODO, soon: we _probably_ don't want allocs/deallocs in an inner particle loop.  Probably.
   if (d.color_r != nullptr) 
   {
      delete d.color_r;
   }
   if (d.color_g != nullptr)
   {
      delete d.color_g;
   }
   if (d.color_b != nullptr)
   {
      delete d.color_b;
   }
   if (d.color_a != nullptr) 
   {
      delete d.color_a;
   }

   d.color_r = data.particle.color.over_lifetime_r->clone();
   d.color_g = data.particle.color.over_lifetime_g->clone();
   d.color_b = data.particle.color.over_lifetime_b->clone();
   d.color_a = data.particle.color.over_lifetime_a->clone();

   d.startSpeed = data.particle.speed.start->nextValue(_curEmitterTime);
   d.currentSpeed = d.startSpeed;
   if (d.speed != nullptr) 
   {
      delete d.speed;
   }
   d.speed = data.particle.speed.over_lifetime->clone();

   d.startScale = data.particle.scale.start->nextValue(_curEmitterTime);

   if (d.scale != nullptr) {
      delete d.scale;
   }
   d.scale = data.particle.scale.over_lifetime->clone();

   if (d.rotation_x != nullptr)
   {
      delete d.rotation_x;
   }
   if (d.rotation_y != nullptr)
   {
      delete d.rotation_y;
   }
   if (d.rotation_z != nullptr)
   {
      delete d.rotation_z;
   }

   d.rotation_x = data.particle.rotation.over_lifetime_x->clone();
   d.rotation_y = data.particle.rotation.over_lifetime_y->clone();
   d.rotation_z = data.particle.rotation.over_lifetime_z->clone();

   if (d.velocity_x != nullptr)
   {
      delete d.velocity_x;
   }
   if (d.velocity_y != nullptr)
   {
      delete d.velocity_y;
   }
   if (d.velocity_z != nullptr)
   {
      delete d.velocity_z;
   }

   d.velocity_x = data.particle.velocity.over_lifetime_x->clone();
   d.velocity_y = data.particle.velocity.over_lifetime_y->clone();
   d.velocity_z = data.particle.velocity.over_lifetime_z->clone();

   ca.matrix = Matrix4f::TransMat(d.position.x, d.position.y, d.position.z);
   ca.matrix.scale(d.startScale, d.startScale, d.startScale);
   ca.color = d.currentColor;
}

void CubemitterNode::updateCube(CubeData &d, CubeAttribute &ca)
{
   if (d.currentLife <= 0) {
      // Set the scale to zero, so nothing is rasterized (burning vertex ops should be fine).
      ca.matrix.scale(0, 0, 0);
      return;
   }
   CubemitterData data = _cubemitterRes.getPtr()->emitterData;

   float fr = 1.0f - (d.currentLife / d.maxLife);

   d.position += d.direction * d.currentSpeed * _timeDelta;
   d.position.x += d.velocity_x->nextValue(fr) * _timeDelta;
   d.position.y += d.velocity_y->nextValue(fr) * _timeDelta;
   d.position.z += d.velocity_z->nextValue(fr) * _timeDelta;

   d.currentLife -= _timeDelta;
   d.currentColor.x = d.color_r->nextValue(fr);
   d.currentColor.y = d.color_g->nextValue(fr);
   d.currentColor.z = d.color_b->nextValue(fr);
   d.currentColor.w = d.color_a->nextValue(fr);
   d.currentScale = d.startScale * d.scale->nextValue(fr);
   d.currentSpeed = d.startSpeed * d.speed->nextValue(fr);
   
   float rotX = degToRad(d.rotation_x->nextValue(fr));
   float rotY = degToRad(d.rotation_y->nextValue(fr));
   float rotZ = degToRad(d.rotation_z->nextValue(fr));


   Matrix4f rot = Matrix4f::RotMat(rotX, rotY, rotZ);
   Matrix4f scale = Matrix4f::ScaleMat(d.currentScale, d.currentScale, d.currentScale);
   rot = rot * scale;

   // This is our actual vbo data.
   ca.matrix.x[0] = rot.x[0];
   ca.matrix.x[1] = rot.x[1];
   ca.matrix.x[2] = rot.x[2];
   ca.matrix.x[4] = rot.x[4];
   ca.matrix.x[5] = rot.x[5];
   ca.matrix.x[6] = rot.x[6];
   ca.matrix.x[8] = rot.x[8];
   ca.matrix.x[9] = rot.x[9];
   ca.matrix.x[10] = rot.x[10];
   ca.matrix.x[12] = d.position.x;
   ca.matrix.x[13] = d.position.y;
   ca.matrix.x[14] = d.position.z;
   ca.color = d.currentColor;
}


/*		// Update bounding box
		Vec3f vertPos( _parPositions[i*3+0], _parPositions[i*3+1], _parPositions[i*3+2] );
		if( vertPos.x < bBMin.x ) bBMin.x = vertPos.x;
		if( vertPos.y < bBMin.y ) bBMin.y = vertPos.y;
		if( vertPos.z < bBMin.z ) bBMin.z = vertPos.z;
		if( vertPos.x > bBMax.x ) bBMax.x = vertPos.x;
		if( vertPos.y > bBMax.y ) bBMax.y = vertPos.y;
		if( vertPos.z > bBMax.z ) bBMax.z = vertPos.z;
	}

	// Avoid zero box dimensions for planes
	if( bBMax.x - bBMin.x == 0 ) bBMax.x += Math::Epsilon;
	if( bBMax.y - bBMin.y == 0 ) bBMax.y += Math::Epsilon;
	if( bBMax.z - bBMin.z == 0 ) bBMax.z += Math::Epsilon;
	
	_bBox.min = bBMin;
	_bBox.max = bBMax;

	_timeDelta = 0;
	_prevAbsTrans = _absTrans;

	timer->setEnabled( false );*/
//}
