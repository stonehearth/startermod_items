#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "egMaterial.h"
#include "egCamera.h"
#include "libjson.h"
#include "lib/json/node.h"

#if defined(ASSERT)
#  undef ASSERT
#endif

#include "extension.h"
#include "datachannel.h"
#include "radiant.h"
#include "cubemitter.h"

using namespace ::radiant;
using namespace ::radiant::json;
using namespace ::radiant::horde3d;

cubemitter::OriginData::SurfaceKind parseSurfaceKind(Node &n)
{
   if (n.as<std::string>() == "POINT") {
      return cubemitter::OriginData::POINT;
   }
   return cubemitter::OriginData::RECTANGLE;
}

cubemitter::OriginData parseOrigin(Node &n) {
   cubemitter::OriginData result;
   if (n.has("surface"))
   {
      result.surfaceKind = parseSurfaceKind(n.get_node("surface"));
   }
   if (result.surfaceKind == cubemitter::OriginData::SurfaceKind::RECTANGLE) {
      if (n.has("values"))
      {
         result.length = n.get_node("values").get<float>(0, 0.0f);
         result.width = n.get_node("values").get<float>(1, 0.0f);
      }
   }
   return result;
}

cubemitter::EmissionData parseEmission(Node& n) 
{
   cubemitter::EmissionData result;
   result.rate = parseChannel(n, "rate", 1.0f);
   result.angle = parseChannel(n, "angle", 25.0f);
   if (n.has("origin"))
   {
      result.origin = parseOrigin(n.get_node("origin"));
   }
   return result;
}

cubemitter::LifetimeData parseLifetime(Node& n)
{
   cubemitter::LifetimeData result;
   result.start = parseChannel(n, "start", 5.0f);
   return result;
}

cubemitter::SpeedData parseSpeed(Node& n)
{
   cubemitter::SpeedData result;
   result.start = parseChannel(n, "start", 5.0f);
   result.over_lifetime = parseChannel(n, "over_lifetime", 1.0f);
   return result;
}

cubemitter::RotationData parseRotation(Node& n)
{
   cubemitter::RotationData result;
   result.over_lifetime_x = parseChannel(n, "over_lifetime_x", 0.0f);
   result.over_lifetime_y = parseChannel(n, "over_lifetime_y", 0.0f);
   result.over_lifetime_z = parseChannel(n, "over_lifetime_z", 0.0f);
   return result;
}

cubemitter::VelocityData parseVelocity(Node& n)
{
   cubemitter::VelocityData result;
   result.over_lifetime_x = parseChannel(n, "over_lifetime_x", 0.0f);
   result.over_lifetime_y = parseChannel(n, "over_lifetime_y", 0.0f);
   result.over_lifetime_z = parseChannel(n, "over_lifetime_z", 0.0f);
   return result;
}

cubemitter::ColorData parseColor(Node& n)
{
   cubemitter::ColorData result;
   result.start = parseChannel(n, "start", Vec4f(1, 0, 0, 1));
   result.over_lifetime_r = parseChannel(n, "over_lifetime_r", result.start->nextValue(0).x);
   result.over_lifetime_g = parseChannel(n, "over_lifetime_g", result.start->nextValue(0).y);
   result.over_lifetime_b = parseChannel(n, "over_lifetime_b", result.start->nextValue(0).z);
   result.over_lifetime_a = parseChannel(n, "over_lifetime_a", result.start->nextValue(0).w);
   return result;
}

cubemitter::ScaleData parseScale(Node& n)
{
   cubemitter::ScaleData result;
   result.start = parseChannel(n, "start", 1.0f);
   result.over_lifetime = parseChannel(n, "over_lifetime", 1.0f);
   return result;
}

cubemitter::ParticleData parseParticle(Node& n) 
{
   cubemitter::ParticleData result;
   if (n.has("speed"))
   {
      result.speed = parseSpeed(n.get_node("speed"));
   }

   if (n.has("lifetime"))
   {
      result.lifetime = parseLifetime(n.get_node("lifetime"));
   }

   if (n.has("color"))
   {
      result.color = parseColor(n.get_node("color"));
   }

   if (n.has("scale"))
   {
      result.scale = parseScale(n.get_node("scale"));
   }

   if (n.has("rotation"))
   {
      result.rotation = parseRotation(n.get_node("rotation"));
   }

   if (n.has("velocity"))
   {
      result.velocity = parseVelocity(n.get_node("velocity"));
   }
   return result;
}


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
   Node root(libjson::parse(jsonData));

   emitterData.duration = root.get("duration", 10.0f);

   emitterData.loops = root.get("loops", true);

   if (root.has("particle"))
   {
      emitterData.particle = parseParticle(root.get_node("particle"));
   }
   if (root.has("emission"))
   {
      emitterData.emission = parseEmission(root.get_node("emission"));
   }
	return true;
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
   _active = true;
   _wasVisible = true;

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
   if (_curEmitterTime > _cubemitterRes->emitterData.duration) {
      if (_cubemitterRes->emitterData.loops) {
         _curEmitterTime -= _cubemitterRes->emitterData.duration;
      } else {
         // Does a "soft" shutdown; let current cubes die, but don't emit new ones.
         stop();
      }
   }

   if (_wasVisible)
   {
	   _timeDelta += timeDelta;
   	markDirty();
   }
}

// Stops production of new particles, but lets the remainder finish up.  Horde will reap the node
// once all particles are gone.
void CubemitterNode::stop()
{
   _active = false;
}

bool CubemitterNode::hasFinished()
{
   if (_active) {
      return false;
   }
	for( uint32 i = 0; i < _maxCubes; ++i )
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

	// Loop through and find all Cubemitters.
	for( const auto &entry : Modules::sceneMan().getRenderableQueue() )
	{
      if( entry.type != SNT_CubemitterNode ) continue; 
		
		CubemitterNode *emitter = (CubemitterNode *)entry.node;

      if( emitter->_maxCubes == 0 ) continue;
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
                  emitter->_wasVisible = false;
						continue;
					}
					else
						queryObj = emitter->_occQueries[occSet];
				}
			}
		}
		
		// Set material
		if( curMatRes != emitter->_materialRes )
		{
			if( !Modules::renderer().setMaterial( emitter->_materialRes, shaderContext ) ) {
            continue;
         }
			curMatRes = emitter->_materialRes;
		}

	   if( queryObj )
		   gRDI->beginQuery( queryObj );

      if (gRDI->getCaps().hasInstancing) {
         emitter->renderWithInstancing();
      } else {
         emitter->renderWithBatches();
      }
      
      emitter->_wasVisible = true;

		if( queryObj )
			gRDI->endQuery( queryObj );
	}

	timer->endQuery();

   // Draw occlusion proxies
	if( occSet >= 0 )
		Modules::renderer().drawOccProxies( 0 );
	
	gRDI->setVertexLayout( 0 );
}

void CubemitterNode::renderWithInstancing()
{
   // Set vertex layout
   gRDI->setVertexLayout( Extension::getCubemitterCubeVL() );

	// Bind cube geometry
   gRDI->setVertexBuffer( 0, Extension::getCubemitterCubeVBO(), 0, sizeof( CubeVert ) );
   gRDI->setIndexBuffer( Extension::getCubemitterCubeIBO(), IDXFMT_16 );

   // Shader uniforms
	ShaderCombination *curShader = Modules::renderer().getCurShader();

   gRDI->updateBufferData(_attributeBuf, 0, sizeof(CubeAttribute) * _maxCubes, _attributesBuff);
   gRDI->setVertexBuffer(1, _attributeBuf, 0, sizeof(CubeAttribute));
   gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, 36, 0, _maxCubes);
}

void CubemitterNode::renderWithBatches()
{
   // Set vertex layout
   gRDI->setVertexLayout(Extension::getCubemitterBatchCubeVL());

   gRDI->setVertexBuffer(0, Extension::getCubemitterBatchCubeVBO(), 0, sizeof(CubeBatchVert));
   gRDI->setIndexBuffer(Extension::getCubemitterBatchCubeIBO(), IDXFMT_16);

   // Shader uniforms
	ShaderCombination *curShader = Modules::renderer().getCurShader();

   uint32 batchNum = 0;
   for (batchNum; batchNum < _maxCubes / CubesPerBatch; batchNum++)
   {
      renderBatch(curShader, batchNum, CubesPerBatch);
   }
   uint32 count = _maxCubes % CubesPerBatch;
	if(count > 0)
   {
      renderBatch(curShader, batchNum, count);
   }
}

void CubemitterNode::renderBatch(ShaderCombination *curShader, int batchNum, int count)
{
   float matArray[16 * CubesPerBatch];
   float colorArray[4 * CubesPerBatch];

   for (int i = 0; i < count; i++)
   {
      memcpy(matArray + (i * 16), _attributesBuff[(CubesPerBatch * batchNum) + i].matrix.x, 16 * sizeof(float));
      colorArray[i * 4 + 0] = _attributesBuff[(CubesPerBatch * batchNum) + i].color.x;
      colorArray[i * 4 + 1] = _attributesBuff[(CubesPerBatch * batchNum) + i].color.y;
      colorArray[i * 4 + 2] = _attributesBuff[(CubesPerBatch * batchNum) + i].color.z;
      colorArray[i * 4 + 3] = _attributesBuff[(CubesPerBatch * batchNum) + i].color.w;
   }

	if (curShader->uni_cubeBatchTransformArray >= 0)
   {
      gRDI->setShaderConst(curShader->uni_cubeBatchTransformArray, CONST_FLOAT44, matArray, count);
   }
	if (curShader->uni_cubeBatchColorArray >= 0)
   {
      gRDI->setShaderConst(curShader->uni_cubeBatchColorArray, CONST_FLOAT4, colorArray, count);
   }
   gRDI->drawIndexed(PRIM_TRILIST, 0, count * 36, 0, count * 8);

   Modules::stats().incStat( EngineStats::BatchCount, 1 );
   Modules::stats().incStat( EngineStats::TriCount, count * 12.0f );
}

void CubemitterNode::onPostUpdate()
{	
   if( _timeDelta <= 0 || !_wasVisible /*|| _effectRes == 0x0*/ ) return;
	
	Timer *timer = Modules::stats().getTimer( EngineStats::ParticleSimTime );
	if( Modules::config().gatherTimeStats ) timer->setEnabled( true );
	
   CubemitterData d = _cubemitterRes.getPtr()->emitterData;

   float timeAvailableToSpawn = _timeDelta - _nextSpawnTime;
   _nextSpawnTime -= _timeDelta;
   int numberToSpawn = 0;
   while (_active && timeAvailableToSpawn > 0) 
   {
      _nextSpawnTime = 1.0f / d.emission.rate->nextValue(_curEmitterTime);
      timeAvailableToSpawn -= _nextSpawnTime;
      numberToSpawn++;
   }
   
   updateAndSpawnCubes(numberToSpawn);
   
   _timeDelta = 0.0f;
   _wasVisible = false;
   timer->setEnabled(false);
}

void CubemitterNode::updateAndSpawnCubes(int numToSpawn) 
{
   _bBox.clear();

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

      if (d.currentLife > 0) 
      {
         _bBox.addPoint(d.position);
      }
   }
}

void CubemitterNode::spawnCube(CubeData &d, CubeAttribute &ca)
{
   CubemitterData data = _cubemitterRes.getPtr()->emitterData;

   d.maxLife = data.particle.lifetime.start->nextValue(_curEmitterTime);
   d.currentLife = d.maxLife;

   Matrix4f m = _absTrans;
   d.position = m.getTrans();

   if (data.emission.origin.surfaceKind == cubemitter::OriginData::SurfaceKind::RECTANGLE)
   {
      float randWidth = randomF(-data.emission.origin.width / 2.0f, data.emission.origin.width / 2.0f);
      float randLength = randomF(-data.emission.origin.length / 2.0f, data.emission.origin.length / 2.0f);
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

void CubemitterNode::updateCube(CubeData& d, CubeAttribute& ca)
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

   // This is our actual vbo data.
   ca.matrix = rot * scale;
   ca.matrix.x[12] = d.position.x;
   ca.matrix.x[13] = d.position.y;
   ca.matrix.x[14] = d.position.z;
   ca.color = d.currentColor;
}