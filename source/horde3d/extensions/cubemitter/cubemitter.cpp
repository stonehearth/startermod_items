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
   JSONNode root;
   try {
      root = libjson::parse(jsonData);
   } catch (std::invalid_argument &ia) {
      return raiseError( "JSON parsing error" );
   }

   auto croot = radiant::json::ConstJsonObject(root);
   emitterData.duration = croot.get("duration", 10.0f);

   emitterData.particle = parseParticle(root.at("particle"));
   emitterData.emission = parseEmission(root.at("emission"));
	return true;
}

EmissionData CubemitterResource::parseEmission(JSONNode& n) {
   EmissionData result;
   result.rate = parseDataChannel(n.at("rate"));
   return result;
}

ParticleData CubemitterResource::parseParticle(JSONNode& n) {
   ParticleData result;

   result.start_lifetime = parseDataChannel(n.at("start_lifetime"));
   result.start_speed = parseDataChannel(n.at("start_speed"));
   result.color = parseColor(n.at("color"));
   result.scale = parseScale(n.at("scale"));
   return result;
}

ColorData CubemitterResource::parseColor(JSONNode& n) {
   ColorData result;
   result.start = parseDataChannel(n.at("start"));
   //result.over_lifetime = parseDataChannel(n.at("over_lifetime"));
   return result;
}

ScaleData CubemitterResource::parseScale(JSONNode& n) {
   ScaleData result;
   result.start = parseDataChannel(n.at("start"));
   result.over_lifetime = parseDataChannel(n.at("over_lifetime"));
   return result;
}

DataChannel CubemitterResource::parseDataChannel(JSONNode& n) {
   DataChannel result;
   result.dataKind = DataChannel::DataKind::SCALAR;
   result.kind = DataChannel::Kind::CONSTANT;
   result.values.push_back(DataChannel::ChannelValue(1));

   if (!n.empty()) {
      auto cn = radiant::json::ConstJsonObject(n);
      result.kind = parseChannelKind(cn.get("kind", std::string("CONSTANT")));
      result.values = parseChannelValues(n.at("values"));
      result.dataKind = extractDataKind(n.at("values"));
   }

   return result;
}

std::vector<DataChannel::ChannelValue> CubemitterResource::parseChannelValues(JSONNode& n) {
   std::vector<DataChannel::ChannelValue> result;
   for (auto const& v : n) {
      // For now, we only handle fixed sizes.
      int s = v.size();
      if (v.size() == 0) {
         result.push_back(DataChannel::ChannelValue(v.as_float()));
      } else if (v.size() == 3) {
         result.push_back(DataChannel::ChannelValue(v.at(0).as_float(), v.at(1).as_float(), v.at(2).as_float()));
      } else if (v.size() == 2) {
         result.push_back(DataChannel::ChannelValue(v.at(0).as_float(), v.at(1).as_float()));
      }
   }
   return result;
}

DataChannel::DataKind CubemitterResource::extractDataKind(JSONNode& n) {
   if (n.size() > 0) {
      if (n[0].size() == 3) {
         return DataChannel::DataKind::TRIPLE;
      } else if (n[0].size() == 2) {
         return DataChannel::DataKind::DOUBLE;
      }
   }
   return DataChannel::DataKind::SCALAR;
}


DataChannel::Kind CubemitterResource::parseChannelKind(std::string& kindName) {
   if (kindName == "CONSTANT") {
      return DataChannel::Kind::CONSTANT;
   } else if (kindName == "RANDOM_BETWEEN") {
      return DataChannel::Kind::RANDOM_BETWEEN;
   } else if (kindName == "CURVE") {
      return DataChannel::Kind::CURVE;
   }
   return DataChannel::Kind::CONSTANT;
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

   for (int i = 0; i < _maxCubes; i++) {
      _attributesBuff[i].matrix.toIdentity();
      _cubes[i].currentLife = 0.0f;
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
	delete[] _parPositions;
	delete[] _parSizesANDRotations;
	delete[] _parColors;
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

Vec3f randomV ( const Vec3f &min, const Vec3f &max ) {
   Vec3f result;
   result.x = randomF(min.x, max.x);
   result.y = randomF(min.y, max.y);
   result.z = randomF(min.z, max.z);

   return result;
}


void CubemitterNode::advanceTime( float timeDelta )
{
	_timeDelta += timeDelta;

	markDirty();
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



		/*if( queryObj )
			gRDI->endQuery( queryObj );*/
	}

	timer->endQuery();

	// Draw occlusion proxies
	//if( occSet >= 0 )
	//	Modules::renderer().drawOccProxies( 0 );
	
	gRDI->setVertexLayout( 0 );
}

DataChannel::ChannelValue CubemitterNode::nextValue(float t, DataChannel dc) {
   if (dc.kind == DataChannel::Kind::CONSTANT) {
      return dc.values[0];
   }

   if (dc.kind == DataChannel::Kind::RANDOM_BETWEEN) {
      if (dc.dataKind == DataChannel::DataKind::SCALAR) {
         float r0 = dc.values[0].value.scalar;
         float r1 = dc.values[1].value.scalar;
         return DataChannel::ChannelValue(randomF(r0, r1));
      } else if (dc.dataKind == DataChannel::DataKind::TRIPLE) {
         Vec3f v1(dc.values[0].value.triple.v0, dc.values[0].value.triple.v1, dc.values[0].value.triple.v2);
         Vec3f v2(dc.values[1].value.triple.v0, dc.values[1].value.triple.v1, dc.values[1].value.triple.v2);
         Vec3f r = randomV(v1, v2);
         return DataChannel::ChannelValue(r.x, r.y, r.z);
      }
   } else if (dc.kind == DataChannel::Kind::CURVE) {
      if (dc.dataKind == DataChannel::DataKind::DOUBLE) {
         float t_start = dc.values[0].value.duble.v0;
         float v_start = dc.values[0].value.duble.v1;

         float t_end = dc.values[1].value.duble.v0;
         float v_end = dc.values[1].value.duble.v1;

         int i = 2;
         while (t_end <= t && i < dc.values.size()) {
            t_start = t_end;
            v_start = v_end;

            t_end = dc.values[i].value.duble.v0;
            v_end = dc.values[i].value.duble.v1;
            i++;
         }

         float frac = (t - t_start) / (t_end - t_start);

         return DataChannel::ChannelValue(frac * v_end + (1.0f - frac) * v_start);
      }
   }

   return DataChannel::ChannelValue(0);
}


void CubemitterNode::onPostUpdate()
{	
	if( _timeDelta <= 0 /*|| _effectRes == 0x0*/ ) return;
	
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
      _nextSpawnTime = 1.0f / nextValue(_curEmitterTime, d.emission.rate).value.scalar;
      timeAvailableToSpawn -= _nextSpawnTime;
      numberToSpawn++;
   }
   
   updateAndSpawnCubes(numberToSpawn);
   
   _timeDelta = 0.0f;
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

   d.maxLife = nextValue(_curEmitterTime, data.particle.start_lifetime).value.scalar;
   d.currentLife = d.maxLife;

   Matrix4f m = _absTrans;
   d.position = m.getTrans();

   float angle = degToRad( 25.0f / 2 );
   m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0;
   m.rotate( randomF( -angle, angle ), randomF( -angle, angle ), randomF( -angle, angle ) );
   d.direction = (m * Vec3f( 0, 0, -1 )).normalized();

   auto startCol = nextValue(_curEmitterTime, data.particle.color.start).value.triple;
   d.color = Vec4f(startCol.v0, startCol.v1, startCol.v2, 1);

   d.speed = nextValue(_curEmitterTime, data.particle.start_speed).value.scalar;

   d.startScale = nextValue(_curEmitterTime, data.particle.scale.start).value.scalar;
   d.scale = d.startScale;

   ca.matrix = Matrix4f::TransMat(d.position.x, d.position.y, d.position.z);
   ca.matrix.scale(d.scale, d.scale, d.scale);
   ca.color = d.color;
}

void CubemitterNode::updateCube(CubeData &d, CubeAttribute &ca)
{
   if (d.currentLife <= 0) {
      // Set the scale to zero, so nothing is rasterized (burning vertex ops should be fine).
      ca.matrix.x[0] = 0; ca.matrix.x[5] = 0; ca.matrix.x[10] = 0;
      return;
   }
   CubemitterData data = _cubemitterRes.getPtr()->emitterData;

   float fr = 1.0f - (d.currentLife / d.maxLife);

   d.position += d.direction * d.speed * _timeDelta;
   d.currentLife -= _timeDelta;
   d.scale = d.startScale * nextValue(fr, data.particle.scale.over_lifetime).value.scalar;


   // This is our actual vbo data.
   ca.matrix.x[12] = d.position.x;
   ca.matrix.x[13] = d.position.y;
   ca.matrix.x[14] = d.position.z;
   ca.matrix.x[0] = d.scale;
   ca.matrix.x[5] = d.scale;
   ca.matrix.x[10] = d.scale;
   ca.color = d.color;
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
