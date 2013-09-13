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

   emitterData.duration = radiant::json::get(root, "duration", 10.0f);

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
   return result;
}

ColorData CubemitterResource::parseColor(JSONNode& n) {
   ColorData result;
   result.start = parseDataChannel(n.at("start"));
   return result;
}

DataChannel CubemitterResource::parseDataChannel(JSONNode& n) {
   DataChannel result;
   result.dataKind = DataChannel::DataKind::SCALAR;
   result.kind = DataChannel::Kind::CONSTANT;
   result.values.push_back(DataChannel::ChannelValue(1));

   if (!n.empty()) {
      result.kind = parseChannelKind(radiant::json::get(n, "kind", std::string("CONSTANT")));
      result.values = parseChannelValues(n.at("values"));
      result.dataKind = extractDataKind(result.values);
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
      }
   }
   return result;
}

DataChannel::DataKind CubemitterResource::extractDataKind(std::vector<DataChannel::ChannelValue> &values) {
   if (values.size() == 3) {
      return DataChannel::DataKind::TRIPLE;
   }
   return DataChannel::DataKind::SCALAR;
}


DataChannel::Kind CubemitterResource::parseChannelKind(std::string& kindName) {
   if (kindName == "CONSTANT") {
      return DataChannel::Kind::CONSTANT;
   } else if (kindName == "RANDOM_BETWEEN") {
      return DataChannel::Kind::RANDOM_BETWEEN;
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
	_materialRes = emitterTpl.matRes;
	//_effectRes = emitterTpl.effectRes;
	_particleCount = emitterTpl.maxParticleCount;
	//_respawnCount = emitterTpl.respawnCount;
	//_delay = emitterTpl.delay;
	//_emissionRate = emitterTpl.emissionRate;
	//_spreadAngle = emitterTpl.spreadAngle;
	//_force = Vec3f( emitterTpl.fx, emitterTpl.fy, emitterTpl.fz );

	_timeDelta = 0;
	_emissionAccum = 0;
	_prevAbsTrans = _absTrans;

	_cubes = 0x0;
	_parPositions = 0x0;
	_parSizesANDRotations = 0x0;
	_parColors = 0x0;

	setMaxParticleCount( _particleCount );
}


CubemitterNode::~CubemitterNode()
{
	for( uint32 i = 0; i < _occQueries.size(); ++i )
	{
		if( _occQueries[i] != 0 )
			gRDI->destroyQuery( _occQueries[i] );
	}
	
	delete[] _cubes;
	delete[] _parPositions;
	delete[] _parSizesANDRotations;
	delete[] _parColors;
}


SceneNodeTpl *CubemitterNode::parsingFunc( std::map< std::string, std::string > &attribs )
{
	bool result = true;
	
	std::map< std::string, std::string >::iterator itr;
	CubemitterNodeTpl *emitterTpl = new CubemitterNodeTpl("", 0 );

	/*itr = attribs.find( "material" );
	if( itr != attribs.end() )
	{
		uint32 res = Modules::resMan().addResource( ResourceTypes::Material, itr->second, 0, false );
		if( res != 0 )
			emitterTpl->matRes = (MaterialResource *)Modules::resMan().resolveResHandle( res );
	}
	else result = false;
	itr = attribs.find( "particleEffect" );
	if( itr != attribs.end() )
	{
		uint32 res = Modules::resMan().addResource( ResourceTypes::ParticleEffect, itr->second, 0, false );
		if( res != 0 )
			emitterTpl->effectRes = (ParticleEffectResource *)Modules::resMan().resolveResHandle( res );
	}
	else result = false;
	itr = attribs.find( "maxCount" );
	if( itr != attribs.end() ) emitterTpl->maxParticleCount = atoi( itr->second.c_str() );
   	else result = false;
	itr = attribs.find( "respawnCount" );
	if( itr != attribs.end() ) emitterTpl->respawnCount = atoi( itr->second.c_str() );
	   else result = false;
	itr = attribs.find( "delay" );
	if( itr != attribs.end() ) emitterTpl->delay = (float)atof( itr->second.c_str() );
	itr = attribs.find( "emissionRate" );
	if( itr != attribs.end() ) emitterTpl->emissionRate = (float)atof( itr->second.c_str() );
	itr = attribs.find( "spreadAngle" );
	if( itr != attribs.end() ) emitterTpl->spreadAngle = (float)atof( itr->second.c_str() );
	itr = attribs.find( "forceX" );
	if( itr != attribs.end() ) emitterTpl->fx = (float)atof( itr->second.c_str() );
	itr = attribs.find( "forceY" );
	if( itr != attribs.end() ) emitterTpl->fy = (float)atof( itr->second.c_str() );
	itr = attribs.find( "forceZ" );
	if( itr != attribs.end() ) emitterTpl->fz = (float)atof( itr->second.c_str() );
	
	if( !result )
	{
		delete emitterTpl; emitterTpl = 0x0;
	}*/
	
	return emitterTpl;
}


SceneNode *CubemitterNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SNT_CubemitterNode ) return 0x0;

	return new CubemitterNode( *(CubemitterNodeTpl *)&nodeTpl );
}


void CubemitterNode::setMaxParticleCount( radiant::uint32 maxParticleCount )
{
	// Delete particles
	delete[] _cubes; _cubes = 0x0;
	delete[] _parPositions; _parPositions = 0x0;
	delete[] _parSizesANDRotations; _parSizesANDRotations = 0x0;
	delete[] _parColors; _parColors = 0x0;
	
	// Initialize particles
	_particleCount = maxParticleCount;
	_cubes = new CubeData[_particleCount];
	_parPositions = new float[_particleCount * 3];
	_parSizesANDRotations = new float[_particleCount * 2];
	_parColors = new float[_particleCount * 4];
	for( uint32 i = 0; i < _particleCount; ++i )
	{
		_cubes[i].life = 0;
		_cubes[i].respawnCounter = 0;
		
		_parPositions[i*3+0] = 0.0f;
		_parPositions[i*3+1] = 0.0f;
		_parPositions[i*3+2] = 0.0f;
		_parSizesANDRotations[i*2+0] = 0.0f;
		_parSizesANDRotations[i*2+1] = 0.0f;
		_parColors[i*4+0] = 0.0f;
		_parColors[i*4+1] = 0.0f;
		_parColors[i*4+2] = 0.0f;
		_parColors[i*4+3] = 0.0f;
	}
}


int CubemitterNode::getParamI( int param )
{
	switch( param )
	{
	/*case CubemitterNodeParams::MatResI:
		if( _materialRes != 0x0 ) return _materialRes->getHandle();
		else return 0;
	case EmitterNodeParams::PartEffResI:
		if( _effectRes != 0x0 ) return _effectRes->getHandle();
		else return 0;*/
	case CubemitterNodeParams::MaxCountI:
		return (int)_particleCount;
	/*case EmitterNodeParams::RespawnCountI:
		return _respawnCount;*/
	}

	return SceneNode::getParamI( param );
}


void CubemitterNode::setParamI( int param, int value )
{
	switch( param )
	{
	/*case EmitterNodeParams::MatResI:
		res = Modules::resMan().resolveResHandle( value );
		if( res != 0x0 && res->getType() == ResourceTypes::Material )
			_materialRes = (MaterialResource *)res;
		else
			Modules::setError( "Invalid handle in h3dSetNodeParamI for H3DEmitter::MatResI" );
		return;
	case EmitterNodeParams::PartEffResI:
		res = Modules::resMan().resolveResHandle( value );
		if( res != 0x0 && res->getType() == ResourceTypes::ParticleEffect )
			_effectRes = (ParticleEffectResource *)res;
		else
			Modules::setError( "Invalid handle in h3dSetNodeParamI for H3DLight::PartEffResI" );
		return;*/
	case CubemitterNodeParams::MaxCountI:
		setMaxParticleCount( (uint32)value );
		return;
	/*case EmitterNodeParams::RespawnCountI:
		_respawnCount = value;
		return;*/
	}

	SceneNode::setParamI( param, value );
}


float CubemitterNode::getParamF( int param, int compIdx )
{
	/*switch( param )
	{
	case EmitterNodeParams::DelayF:
		return _delay;
	case EmitterNodeParams::EmissionRateF:
		return _emissionRate;
	case EmitterNodeParams::SpreadAngleF:
		return _spreadAngle;
	case EmitterNodeParams::ForceF3:
		if( (unsigned)compIdx < 3 ) return _force[compIdx];
		break;
	}*/

	return SceneNode::getParamF( param, compIdx );
}


void CubemitterNode::setParamF( int param, int compIdx, float value )
{
	/*switch( param )
	{
	case EmitterNodeParams::DelayF:
		_delay = value;
		return;
	case EmitterNodeParams::EmissionRateF:
		_emissionRate = value;
		return;
	case EmitterNodeParams::SpreadAngleF:
		_spreadAngle = value;
		return;
	case EmitterNodeParams::ForceF3:
		if( (unsigned)compIdx < 3 )
		{
			_force[compIdx] = value;
			return;
		}
		break;
	}*/

	SceneNode::setParamF( param, compIdx, value );
}


float randomF( float min, float max )
{
	return (rand() / (float)RAND_MAX) * (max - min) + min;
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
		if( _cubes[i].life > 0 || (int)_cubes[i].respawnCounter < _respawnCount )
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
	gRDI->setVertexBuffer( 0, Modules::renderer().getCubeVBO(), 0, sizeof( CubeVert ) );
   gRDI->setIndexBuffer( Modules::renderer().getCubeIdxBuf(), IDXFMT_16 );

	// Loop through Cubemitter queue
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
      gRDI->setVertexLayout( Modules::renderer()._vlCube );
		
		//if( queryObj )
		//	gRDI->beginQuery( queryObj );
		
		// Shader uniforms
		ShaderCombination *curShader = Modules::renderer().getCurShader();
		if( curShader->uni_nodeId >= 0 )
		{
			float id = (float)emitter->getHandle();
			gRDI->setShaderConst( curShader->uni_nodeId, CONST_FLOAT, &id );
		}

      float mat[32] = { 
         1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         0, 10, 0, 1,

         1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         10, 20, 10, 1 
      };
      gRDI->updateBufferData(Modules::renderer()._attributeBuf, 0, sizeof(float) * 16 * 2, mat);
      gRDI->setVertexBuffer(1, Modules::renderer()._attributeBuf, 0, sizeof(float) * 16);
      gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, 36, 0, 2);

		// Divide particles in batches and render them
		/*for( uint32 j = 0; j < emitter->_particleCount / ParticlesPerBatch; ++j )
		{
			// Check if batch needs to be rendered
			bool allDead = true;
			for( uint32 k = 0; k < ParticlesPerBatch; ++k )
			{
				if( emitter->_cubes[j*ParticlesPerBatch + k].life > 0 )
				{
					allDead = false;
					break;
				}
			}
			if( allDead ) continue;

			// Render batch
			if( curShader->uni_parPosArray >= 0 )
				gRDI->setShaderConst( curShader->uni_parPosArray, CONST_FLOAT3,
				                      (float *)emitter->_parPositions + j*ParticlesPerBatch*3, ParticlesPerBatch );
			if( curShader->uni_parSizeAndRotArray >= 0 )
				gRDI->setShaderConst( curShader->uni_parSizeAndRotArray, CONST_FLOAT2,
				                      (float *)emitter->_parSizesANDRotations + j*ParticlesPerBatch*2, 
                                  ParticlesPerBatch );
			if( curShader->uni_parColorArray >= 0 )
				gRDI->setShaderConst( curShader->uni_parColorArray, CONST_FLOAT4,
				                      (float *)emitter->_parColors + j*ParticlesPerBatch*4, ParticlesPerBatch );

			gRDI->drawIndexed( PRIM_TRILIST, 0, ParticlesPerBatch * 6, 0, ParticlesPerBatch * 4 );
			Modules::stats().incStat( EngineStats::BatchCount, 1 );
			Modules::stats().incStat( EngineStats::TriCount, ParticlesPerBatch * 2.0f );
		}*/

		/*uint32 count = emitter->_particleCount % ParticlesPerBatch;
		if( count > 0 )
		{
			uint32 offset = (emitter->_particleCount / ParticlesPerBatch) * ParticlesPerBatch;
			
			// Check if batch needs to be rendered
			bool allDead = true;
			for( uint32 k = 0; k < count; ++k )
			{
				if( emitter->_cubes[offset + k].life > 0 )
				{
					allDead = false;
					break;
				}
			}
			
			if( !allDead )
			{
				// Render batch
				if( curShader->uni_parPosArray >= 0 )
					gRDI->setShaderConst( curShader->uni_parPosArray, CONST_FLOAT3,
					                      (float *)emitter->_parPositions + offset*3, count );
				if( curShader->uni_parSizeAndRotArray >= 0 )
					gRDI->setShaderConst( curShader->uni_parSizeAndRotArray, CONST_FLOAT2,
					                      (float *)emitter->_parSizesANDRotations + offset*2, count );
				if( curShader->uni_parColorArray >= 0 )
					gRDI->setShaderConst( curShader->uni_parColorArray, CONST_FLOAT4,
					                      (float *)emitter->_parColors + offset*4, count );
				
				gRDI->drawIndexed( PRIM_TRILIST, 0, count * 6, 0, count * 4 );
				Modules::stats().incStat( EngineStats::BatchCount, 1 );
				Modules::stats().incStat( EngineStats::TriCount, count * 2.0f );
			}
		}

		if( queryObj )
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
	if( _timeDelta == 0 /*|| _effectRes == 0x0*/ ) return;
	
	/*Timer *timer = Modules::stats().getTimer( EngineStats::ParticleSimTime );
	if( Modules::config().gatherTimeStats ) timer->setEnabled( true );
	
	Vec3f bBMin( Math::MaxFloat, Math::MaxFloat, Math::MaxFloat );
	Vec3f bBMax( -Math::MaxFloat, -Math::MaxFloat, -Math::MaxFloat );
	
	if( _delay <= 0 )
		_emissionAccum += _emissionRate * _timeDelta;
	else
		_delay -= _timeDelta;

	Vec3f motionVec = _absTrans.getTrans() - _prevAbsTrans.getTrans();

	// Check how many particles will be spawned
	float spawnCount = 0;
	for( uint32 i = 0; i < _particleCount; ++i )
	{
		CubeData &p = _cubes[i];
		if( p.life <= 0 && ((int)p.respawnCounter < _respawnCount || _respawnCount < 0) )
		{
			spawnCount += 1.0f;
			if( spawnCount >= _emissionAccum ) break;
		}
	}

	// Particles are distributed along emitter's motion vector to avoid blobs when fps is low
	float curStep = 0, stepWidth = 0.5f;
	if( spawnCount > 2.0f ) stepWidth = motionVec.length() / spawnCount;
	
	for( uint32 i = 0; i < _particleCount; ++i )
	{
		CubeData &p = _cubes[i];
		
		// Create particle
		if( p.life <= 0 && ((int)p.respawnCounter < _respawnCount || _respawnCount < 0) )
		{
			if( _emissionAccum >= 1.0f )
			{
				// Respawn
				p.maxLife = randomF( _effectRes->_lifeMin, _effectRes->_lifeMax );
				p.life = p.maxLife;
				float angle = degToRad( _spreadAngle / 2 );
				Matrix4f m = _absTrans;
				m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0;
				m.rotate( randomF( -angle, angle ), randomF( -angle, angle ), randomF( -angle, angle ) );
				p.dir = (m * Vec3f( 0, 0, -1 )).normalized();
				p.dragVec = motionVec / _timeDelta;
				++p.respawnCounter;

				// Generate start values
				p.moveVel0 = randomF( _effectRes->_moveVel.startMin, _effectRes->_moveVel.startMax );
				p.rotVel0 = randomF( _effectRes->_rotVel.startMin, _effectRes->_rotVel.startMax );
				p.drag0 = randomF( _effectRes->_drag.startMin, _effectRes->_drag.startMax );
				p.size0 = randomF( _effectRes->_size.startMin, _effectRes->_size.startMax );
				p.r0 = randomF( _effectRes->_colR.startMin, _effectRes->_colR.startMax );
				p.g0 = randomF( _effectRes->_colG.startMin, _effectRes->_colG.startMax );
				p.b0 = randomF( _effectRes->_colB.startMin, _effectRes->_colB.startMax );
				p.a0 = randomF( _effectRes->_colA.startMin, _effectRes->_colA.startMax );
				
				// Update arrays
				_parPositions[i * 3 + 0] = _absTrans.c[3][0] - motionVec.x * curStep;
				_parPositions[i * 3 + 1] = _absTrans.c[3][1] - motionVec.y * curStep;
				_parPositions[i * 3 + 2] = _absTrans.c[3][2] - motionVec.z * curStep;
				_parSizesANDRotations[i * 2 + 0] = p.size0;
				_parSizesANDRotations[i * 2 + 1] = randomF( 0, 360 );
				_parColors[i * 4 + 0] = p.r0;
				_parColors[i * 4 + 1] = p.g0;
				_parColors[i * 4 + 2] = p.b0;
				_parColors[i * 4 + 3] = p.a0;

				// Update emitter
				_emissionAccum -= 1.f;
				if( _emissionAccum < 0 ) _emissionAccum = 0.f;

				curStep += stepWidth;
			}
		}
		
		// Update particle
		if( p.life > 0 )
		{
			// Interpolate data
			float fac = 1.0f - (p.life / p.maxLife);
			
			float moveVel = p.moveVel0 * (1.0f + (_effectRes->_moveVel.endRate - 1.0f) * fac);
			float rotVel = p.rotVel0 * (1.0f + (_effectRes->_rotVel.endRate - 1.0f) * fac);
			float drag = p.drag0 * (1.0f + (_effectRes->_drag.endRate - 1.0f) * fac);
			_parSizesANDRotations[i * 2 + 0] = p.size0 * (1.0f + (_effectRes->_size.endRate - 1.0f) * fac);
			_parSizesANDRotations[i * 2 + 0] *= 2;  // Keep compatibility with old particle vertex shader
			_parColors[i * 4 + 0] = p.r0 * (1.0f + (_effectRes->_colR.endRate - 1.0f) * fac);
			_parColors[i * 4 + 1] = p.g0 * (1.0f + (_effectRes->_colG.endRate - 1.0f) * fac);
			_parColors[i * 4 + 2] = p.b0 * (1.0f + (_effectRes->_colB.endRate - 1.0f) * fac);
			_parColors[i * 4 + 3] = p.a0 * (1.0f + (_effectRes->_colA.endRate - 1.0f) * fac);

			// Update particle position and rotation
			_parPositions[i * 3 + 0] += (p.dir.x * moveVel + p.dragVec.x * drag + _force.x) * _timeDelta;
			_parPositions[i * 3 + 1] += (p.dir.y * moveVel + p.dragVec.y * drag + _force.y) * _timeDelta;
			_parPositions[i * 3 + 2] += (p.dir.z * moveVel + p.dragVec.z * drag + _force.z) * _timeDelta;
			_parSizesANDRotations[i * 2 + 1] += degToRad( rotVel ) * _timeDelta;

			// Decrease lifetime
			p.life -= _timeDelta;
			
			// Check if particle is dying
			if( p.life <= 0 )
			{
				_parSizesANDRotations[i * 2 + 0] = 0.0f;
			}
		}

		// Update bounding box
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
}
