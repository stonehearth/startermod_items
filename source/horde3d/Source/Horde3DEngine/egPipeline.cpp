// *************************************************************************************************
//
// Horde3D
//   Next-Generation Graphics Engine
// --------------------------------------
// Copyright (C) 2006-2011 Nicolas Schulz
//
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/legal/epl-v10.html
//
// *************************************************************************************************

#include "egPipeline.h"
#include "egMaterial.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "utXML.h"
#include <boost/algorithm/string.hpp>  
#include <fstream>

#include "utDebug.h"

static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize)
{
   if (nsize == 0) {
      free(ptr);
      return NULL;
   }
   return realloc(ptr, nsize);
}

namespace Horde3D {

using namespace std;


PipelineResource::PipelineResource( std::string const& name, int flags ) :
	Resource( ResourceTypes::Pipeline, name, flags )
{
	initDefault();	
}


PipelineResource::~PipelineResource()
{
	release();
}


void PipelineResource::initDefault()
{
	_baseWidth = 320; _baseHeight = 240;
}


void PipelineResource::release()
{
	releaseRenderTargets();
	_renderTargets.clear();
   _globalRenderTargets.clear();
	_stages.clear();
}


bool PipelineResource::raiseError( std::string const& msg, int line )
{
	// Reset
	release();
	initDefault();

	if( line < 0 )
		Modules::log().writeError( "Pipeline resource '%s': %s", _name.c_str(), msg.c_str() );
	else
		Modules::log().writeError( "Pipeline resource '%s' in line %i: %s", _name.c_str(), line, msg.c_str() );
	
	return false;
}


std::string PipelineResource::parseStage( XMLNode const &node, PipelineStagePtr &stage )
{
	stage->id = node.getAttribute( "id", "" );

        std::string lower = stage->id;
        boost::algorithm::to_lower(lower);
        stage->debug_name = "h3d stage " + lower;

	
	if( _stricmp( node.getAttribute( "enabled", "true" ), "false" ) == 0 ||
		_stricmp( node.getAttribute( "enabled", "1" ), "0" ) == 0 )
		stage->enabled = false;
	else
		stage->enabled = true;

	if( strcmp( node.getAttribute( "link", "" ), "" ) != 0 )
	{
		uint32 mat = Modules::resMan().addResource(
			ResourceTypes::Material, node.getAttribute( "link" ), 0, false );
		stage->matLink = (MaterialResource *)Modules::resMan().resolveResHandle( mat );
	}
	
	stage->commands.reserve( node.countChildNodes() );

	// Parse commands
	XMLNode node1 = node.getFirstChild();
	while( !node1.isEmpty() )
	{
		if( strcmp( node1.getName(), "SwitchTarget" ) == 0 )
		{
			if( !node1.getAttribute( "target" ) ) return "Missing SwitchTarget attribute 'target'";
			
			void *renderTarget = 0x0;
			if( strcmp( node1.getAttribute( "target" ), "" ) != 0 )
			{
				renderTarget = findRenderTarget( node1.getAttribute( "target" ) );
				if( !renderTarget ) return "Reference to undefined render target in SwitchTarget";
			}

			stage->commands.push_back( PipelineCommand( PipelineCommands::SwitchTarget ) );
			stage->commands.back().params.resize( 1 );
			stage->commands.back().params[0].setPtr( renderTarget );
		}
		else if( strcmp( node1.getName(), "BindBuffer" ) == 0 )
		{
         const char *sampler = node1.getAttribute( "sampler" );
         const char *sourceRT = node1.getAttribute( "sourceRT" );
         const char *bufIndex = node1.getAttribute( "bufIndex" );
			if (!sampler || !sourceRT || !bufIndex) {
				return "Missing BindBuffer attribute";
         }
			
         int rendBuf = 0;
         RenderTarget *renderTarget = nullptr;
			renderTarget = findRenderTarget(sourceRT);
			if (!renderTarget) {
            return "Reference to undefined render target in BindBuffer";
         }
			
			stage->commands.push_back( PipelineCommand( PipelineCommands::BindBuffer ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 4 );
         params[0].setPtr(renderTarget);
			params[1].setString(sampler);
			params[2].setInt(atoi(bufIndex));
         params[3].setInt(rendBuf);
		}
		else if( strcmp( node1.getName(), "UnbindBuffers" ) == 0 )
		{
			stage->commands.push_back( PipelineCommand( PipelineCommands::UnbindBuffers ) );
		}
		else if( strcmp( node1.getName(), "ClearTarget" ) == 0 )
		{
			stage->commands.push_back( PipelineCommand( PipelineCommands::ClearTarget ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 10 );
			params[0].setBool( false );
			params[1].setBool( false );
			params[2].setBool( false );
			params[3].setBool( false );
			params[4].setBool( false );
			params[5].setFloat( (float)atof( node1.getAttribute( "col_R", "0" ) ) );
			params[6].setFloat( (float)atof( node1.getAttribute( "col_G", "0" ) ) );
			params[7].setFloat( (float)atof( node1.getAttribute( "col_B", "0" ) ) );
			params[8].setFloat( (float)atof( node1.getAttribute( "col_A", "0" ) ) );
			params[9].setInt( atoi( node1.getAttribute( "stencilBuf", "-1" ) ) );
			
			if( _stricmp( node1.getAttribute( "depthBuf", "false" ), "true" ) == 0 ||
			    _stricmp( node1.getAttribute( "depthBuf", "0" ), "1" ) == 0 )
			{
				params[0].setBool( true );
			}
			if( _stricmp( node1.getAttribute( "colBuf0", "false" ), "true" ) == 0 ||
			    _stricmp( node1.getAttribute( "colBuf0", "0" ), "1" ) == 0 )
			{
				params[1].setBool( true );
			}
			if( _stricmp( node1.getAttribute( "colBuf1", "false" ), "true" ) == 0 ||
			    _stricmp( node1.getAttribute( "colBuf1", "0" ), "1" ) == 0 )
			{
				params[2].setBool( true );
			}
			if( _stricmp( node1.getAttribute( "colBuf2", "false" ), "true" ) == 0 ||
			    _stricmp( node1.getAttribute( "colBuf2", "0" ), "1" ) == 0 )
			{
				params[3].setBool( true );
			}
			if( _stricmp( node1.getAttribute( "colBuf3", "false" ), "true" ) == 0 ||
			    _stricmp( node1.getAttribute( "colBuf3", "0" ), "1" ) == 0 )
			{
				params[4].setBool( true );
			}
			if( _stricmp( node1.getAttribute( "depthBuf", "false" ), "true" ) == 0 ||
			    _stricmp( node1.getAttribute( "depthBuf", "0" ), "1" ) == 0 )
			{
				params[0].setBool( true );
			}
	   }
		else if( strcmp( node1.getName(), "DrawGeometry" ) == 0 )
		{
			if( !node1.getAttribute( "context" ) ) return "Missing DrawGeometry attribute 'context'";
			
			const char *orderStr = node1.getAttribute( "order", "" );
			int order = RenderingOrder::StateChanges;
			if( _stricmp( orderStr, "FRONT_TO_BACK" ) == 0 ) order = RenderingOrder::FrontToBack;
			else if( _stricmp( orderStr, "BACK_TO_FRONT" ) == 0 ) order = RenderingOrder::BackToFront;
			else if( _stricmp( orderStr, "NONE" ) == 0 ) order = RenderingOrder::None;

         float frustStart = (float)atof(node1.getAttribute("frustum_start", "0.0"));
         float frustEnd = (float)atof(node1.getAttribute("frustum_end", "1.0"));
         int lodLevel = atoi(node1.getAttribute("forceLodLevel", "-1"));
			
			stage->commands.push_back( PipelineCommand( PipelineCommands::DrawGeometry ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 6 );
			params[0].setString( node1.getAttribute( "context" ) );
			params[1].setInt( order );
			params[2].setInt( 0 );
         params[3].setFloat(frustStart);
         params[4].setFloat(frustEnd);
         params[5].setInt(lodLevel);
		}
		else if( strcmp( node1.getName(), "DrawSelected" ) == 0 )
		{
			if( !node1.getAttribute( "context" ) ) return "Missing DrawSelected attribute 'context'";
			
			const char *orderStr = node1.getAttribute( "order", "" );
			int order = RenderingOrder::StateChanges;
			if( _stricmp( orderStr, "FRONT_TO_BACK" ) == 0 ) order = RenderingOrder::FrontToBack;
			else if( _stricmp( orderStr, "BACK_TO_FRONT" ) == 0 ) order = RenderingOrder::BackToFront;
			else if( _stricmp( orderStr, "NONE" ) == 0 ) order = RenderingOrder::None;
         int lodLevel = atoi(node1.getAttribute("forceLodLevel", "-1"));
			
			stage->commands.push_back( PipelineCommand( PipelineCommands::DrawSelected ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 6 );			
			params[0].setString( node1.getAttribute( "context" ) );
			params[1].setInt( order );
			params[2].setInt( 0 );
         params[3].setFloat(0.0);
         params[4].setFloat(1.0);
         params[5].setInt(lodLevel);
		}
		else if( strcmp( node1.getName(), "DrawProjections" ) == 0 )
		{
			if( !node1.getAttribute( "context" ) ) return "Missing DrawProjections attribute 'context'";
			
         stage->commands.push_back( PipelineCommand( PipelineCommands::DrawProjections ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 2 );
			params[0].setString( node1.getAttribute( "context" ) );
         params[1].setInt( atoi(node1.getAttribute( "userFlags", "0" )) );
		}
		else if( strcmp( node1.getName(), "DrawOverlays" ) == 0 )
		{
			if( !node1.getAttribute( "context" ) ) return "Missing DrawOverlays attribute 'context'";
			
			stage->commands.push_back( PipelineCommand( PipelineCommands::DrawOverlays ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 1 );
			params[0].setString( node1.getAttribute( "context" ) );
		}
		else if( strcmp( node1.getName(), "DrawQuad" ) == 0 )
		{
			if( !node1.getAttribute( "material" ) ) return "Missing DrawQuad attribute 'material'";
			if( !node1.getAttribute( "context" ) ) return "Missing DrawQuad attribute 'context'";
			
			uint32 matRes = Modules::resMan().addResource(
				ResourceTypes::Material, node1.getAttribute( "material" ), 0, false );
			
			stage->commands.push_back( PipelineCommand( PipelineCommands::DrawQuad ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 2 );
			params[0].setResource( Modules::resMan().resolveResHandle( matRes ) );
			params[1].setString( node1.getAttribute( "context" ) );
		}
		else if( strcmp( node1.getName(), "DoForwardLightLoop" ) == 0 )
		{
			const char *orderStr = node1.getAttribute( "order", "" );
			int order = RenderingOrder::StateChanges;
			if( _stricmp( orderStr, "FRONT_TO_BACK" ) == 0 ) order = RenderingOrder::FrontToBack;
			else if( _stricmp( orderStr, "BACK_TO_FRONT" ) == 0 ) order = RenderingOrder::BackToFront;
			else if( _stricmp( orderStr, "NONE" ) == 0 ) order = RenderingOrder::None;

			stage->commands.push_back( PipelineCommand( PipelineCommands::DoForwardLightLoop ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 5 );
			params[0].setString( node1.getAttribute( "suffix", "FORWARD" ) );
			params[1].setBool( _stricmp( node1.getAttribute( "noShadows", "false" ), "true" ) == 0 );
			params[2].setInt( order );
         params[3].setBool( _stricmp( node1.getAttribute( "selectedOnly", "false"), "true" ) == 0 );
         params[4].setInt( atoi(node1.getAttribute( "forceLodLevel", "-1")));
		}
		else if( strcmp( node1.getName(), "DoDeferredLightLoop" ) == 0 )
		{
         stage->commands.push_back( PipelineCommand( PipelineCommands::DoDeferredLightLoop ) );
         vector< PipeCmdParam > &params = stage->commands.back().params;

         uint32 matRes = Modules::resMan().addResource(ResourceTypes::Material, node1.getAttribute("material"), 0, false);
         params.resize(2);
         params[0].setBool(_stricmp(node1.getAttribute("noShadows", "false"), "true") == 0);
         params[1].setResource(Modules::resMan().resolveResHandle(matRes));
		}
		else if( strcmp( node1.getName(), "SetUniform" ) == 0 )
		{
			if( !node1.getAttribute( "material" ) ) return "Missing SetUniform attribute 'material'";
			if( !node1.getAttribute( "uniform" ) ) return "Missing SetUniform attribute 'uniform'";
			
			uint32 matRes = Modules::resMan().addResource(
				ResourceTypes::Material, node1.getAttribute( "material" ), 0, false );
			
			stage->commands.push_back( PipelineCommand( PipelineCommands::SetUniform ) );
			vector< PipeCmdParam > &params = stage->commands.back().params;
			params.resize( 6 );
			params[0].setResource( Modules::resMan().resolveResHandle( matRes ) );
			params[1].setString( node1.getAttribute( "uniform" ) );
			params[2].setFloat( (float)atof( node1.getAttribute( "a", "0" ) ) );
			params[3].setFloat( (float)atof( node1.getAttribute( "b", "0" ) ) );
			params[4].setFloat( (float)atof( node1.getAttribute( "c", "0" ) ) );
			params[5].setFloat( (float)atof( node1.getAttribute( "d", "0" ) ) );
		}
      else if ( strcmp( node1.getName(), "BuildMipmaps" ) == 0 )
      {
         if ( !node1.getAttribute( "rt" ) ) return "Missing BuildMipmaps attribute 'rt'";
         if ( !node1.getAttribute( "index" ) ) return "Missing BuildMipmaps attribute 'index'";

         stage->commands.push_back( PipelineCommand( PipelineCommands::BuildMipmap ) );

         void *renderTarget = findRenderTarget( node1.getAttribute( "rt" ) );
			if( !renderTarget ) return "Reference to undefined render target in BuildMipmaps";

         vector< PipeCmdParam > &params = stage->commands.back().params;
         params.resize( 2 );
         params[0].setPtr(renderTarget);
         params[1].setInt(atoi(node1.getAttribute("index")));
      }

		node1 = node1.getNextSibling();
	}

	return "";
}


void PipelineResource::addGlobalRenderTarget(const char* name)
{
   RenderTarget t;
   t.id = std::string(name);   
   _globalRenderTargets.push_back(t);
}


void PipelineResource::addRenderTarget( std::string const& id, bool depthBuf, uint32 numColBufs,
										TextureFormats::List format, uint32 samples,
                              uint32 width, uint32 height, float scale, uint32 mipLevels, std::vector<RenderTargetAlias>& aliases )
{
	RenderTarget rt;
	
	rt.id = id;
	rt.hasDepthBuf = depthBuf;
	rt.numColBufs = numColBufs;
	rt.format = format;
	rt.samples = samples;
	rt.width = width;
	rt.height = height;
	rt.scale = scale;
   rt.mipLevels = mipLevels;
   rt.aliases = aliases;

	_renderTargets.push_back( rt );
}

RenderTarget *PipelineResource::findRenderTarget( std::string const& id )
{
	if( id.empty() ) return 0x0;
	
	for( uint32 i = 0; i < _renderTargets.size(); ++i )
	{
		if( _renderTargets[i].id == id )
		{
			return &_renderTargets[i];
		}
	}

   for ( auto& rt : _globalRenderTargets) {
      if (rt.id == id) {
         return &rt;
      }
   }
	
	return 0x0;
}


bool PipelineResource::createRenderTargets()
{
   for( uint32 i = 0; i < _renderTargets.size(); ++i )
   {
      RenderTarget &rt = _renderTargets[i];

      // At this point, any aliases this render target may point to MUST be resolved.  So, resolve all aliases.
      std::vector<RenderBufferAlias> resolvedAliases;
      for (RenderTargetAlias& rta : rt.aliases) {
         resolvedAliases.push_back(RenderBufferAlias());

         // Note: if rt aliases a depth buffer, then the last 'extra' RenderTargetAlias in the vector is the depth buffer.
         // Render buffer creation will automatically take care of it for us, so there's nothing special to do to
         // handle it.
         if (rta.aliasIdx != -1) {
            for (uint32 j = 0; j < i; j++) {
               if (_renderTargets[j].id == rta.targetAlias) {
                  resolvedAliases.back().index = rta.aliasIdx;
                  resolvedAliases.back().robj = _renderTargets[j].rendBuf;
                  break;
               }
            }
         }
      }

      uint32 width = ftoi_r( rt.width * rt.scale ), height = ftoi_r( rt.height * rt.scale );
      if( width == 0 ) width = ftoi_r( _baseWidth * rt.scale );
      if( height == 0 ) height = ftoi_r( _baseHeight * rt.scale );

      rt.rendBuf = gRDI->createRenderBufferWithAliases(
         width, height, rt.format, rt.hasDepthBuf, rt.numColBufs, rt.samples, resolvedAliases, rt.mipLevels);
      if( rt.rendBuf == 0 ) return false;
   }

	return true;
}


void PipelineResource::releaseRenderTargets()
{
	for( uint32 i = 0; i < _renderTargets.size(); ++i )
	{
		RenderTarget &rt = _renderTargets[i];

		if( rt.rendBuf )
			gRDI->destroyRenderBuffer( rt.rendBuf );
	}
}


bool PipelineResource::load( const char *data, int size )
{
	if( !Resource::load( data, size ) ) return false;

	XMLDoc doc;
	doc.parseBuffer( data, size );
	if( doc.hasError() )
		return raiseError( "XML parsing error" );

	XMLNode rootNode = doc.getRootNode();
	if( strcmp( rootNode.getName(), "Pipeline" ) != 0 )
		return raiseError( "Not a pipeline resource file" );

   return loadXMLPipeline(rootNode);
}

bool PipelineResource::setupPipeline(std::string const& xml)
{
	XMLDoc doc;
	doc.parseBuffer(xml.c_str(), (int)xml.size());
	if( doc.hasError() ) {
		return raiseError( "Error parsing xml pipeline setup" );
   }
   return loadSetupNode(doc.getRootNode());
}

PipelineStagePtr PipelineResource::compileStage(std::string const& xml)
{
	XMLDoc doc;
	doc.parseBuffer(xml.c_str(), (int)xml.size());
	if( doc.hasError() ) {
		raiseError( "Error parsing xml pipeline stage" );
      return nullptr;
   }
   return compileStageNode(doc.getRootNode());
}

bool PipelineResource::loadSetupNode(XMLNode const& setupNode)
{
	XMLNode node2 = setupNode.getFirstChild( "RenderTarget" );
	while( !node2.isEmpty() )
	{
		if( !node2.getAttribute( "id" ) ) return raiseError( "Missing RenderTarget attribute 'id'" );
		std::string id = node2.getAttribute( "id" );
			
		bool depth = false;
      std::string depthAlias = "";
		
      if (node2.getAttribute("depthBuf", nullptr)) {
   		depth = _stricmp(node2.getAttribute("depthBuf"), "true") == 0;
      } else if (node2.getAttribute("depthBufAlias", nullptr)) {
         depth = true;
         depthAlias = node2.getAttribute("depthBufAlias");
      } else {
         return raiseError("Missing RenderTarget attribute 'depthBuf'");
      }
			
		if( !node2.getAttribute( "numColBufs", nullptr ) ) return raiseError( "Missing RenderTarget attribute 'numColBufs'" );
		uint32 numBuffers = atoi( node2.getAttribute( "numColBufs") );
      std::vector<RenderTargetAlias> aliases;
      for (uint32 i = 0; i < numBuffers; i++) {
         char buff[256];
         sprintf(buff, "colBuf%d", i);

         aliases.push_back(RenderTargetAlias());
         if (node2.getAttribute(buff, nullptr)) {
            char buff2[256];
            strncpy(buff2, node2.getAttribute(buff), 256);

            char * bufName = strtok(buff2, "[");
            aliases.back().targetAlias = bufName;

            int index = atoi(strtok(NULL, "]"));
            aliases.back().aliasIdx = index;
         }
      }

      if (depth) {
         aliases.push_back(RenderTargetAlias());
         if (depthAlias != "") {
            aliases.back().targetAlias = depthAlias;
            aliases.back().aliasIdx = 32;
         }
      }			
		TextureFormats::List format = TextureFormats::BGRA8;
		if( node2.getAttribute( "format", nullptr ) != 0x0 )
		{
			if( _stricmp( node2.getAttribute( "format" ), "RGBA8" ) == 0 )
				format = TextureFormats::BGRA8;
			else if( _stricmp( node2.getAttribute( "format" ), "RGBA16F" ) == 0 )
				format = TextureFormats::RGBA16F;
			else if( _stricmp( node2.getAttribute( "format" ), "RGBA32F" ) == 0 )
				format = TextureFormats::RGBA32F;
			else return raiseError( "Unknown RenderTarget format" );
		}

		int maxSamples = atoi( node2.getAttribute( "maxSamples", "0" ) );

		uint32 width = atoi( node2.getAttribute( "width", "0" ) );
		uint32 height = atoi( node2.getAttribute( "height", "0" ) );
		float scale = (float)atof( node2.getAttribute( "scale", "1" ) );

      uint32 mipLevels = atoi(node2.getAttribute("mipLevels", "0"));

		addRenderTarget( id, depth, numBuffers, format,
			std::min( maxSamples, Modules::config().sampleCount ), width, height, scale, mipLevels, aliases );

		node2 = node2.getNextSibling( "RenderTarget" );
	}


   node2 = setupNode.getFirstChild( "GlobalRenderTarget" );
	while( !node2.isEmpty() )
	{
      if( !node2.getAttribute( "id" ) ) return raiseError( "Missing RenderTarget attribute 'id'" );
      addGlobalRenderTarget(node2.getAttribute( "id" ));

      node2 = node2.getNextSibling( "GlobalRenderTarget" );
   }

   return true;
}

PipelineStagePtr PipelineResource::compileStageNode(XMLNode const& node)
{
   auto stage = std::make_shared<PipelineStage>();
	std::string errorMsg = parseStage(node, stage);

	if( !errorMsg.empty() ) {
		raiseError( "Error in stage '" + stage->id + "': " + errorMsg );
      return nullptr;
   }

   return stage;
}

bool PipelineResource::loadXMLPipeline(XMLNode const& rootNode)
{
   _pipelineName = rootNode.getAttribute("name", "");

	// Parse setup
	XMLNode node1 = rootNode.getFirstChild( "Setup" );
	if( !node1.isEmpty() )
	{
      if (!loadSetupNode(node1)) {
         return false;
      }
   }

	// Parse commands
	node1 = rootNode.getFirstChild( "CommandQueue" );
	if( !node1.isEmpty() )
	{
		_stages.reserve( node1.countChildNodes( "Stage" ) );
		
		XMLNode node2 = node1.getFirstChild( "Stage" );
		while( !node2.isEmpty() )
		{
         PipelineStagePtr stage = compileStageNode(node2);
         if (!stage) {
            return false;
         }
			_stages.push_back(stage);
			node2 = node2.getNextSibling( "Stage" );
		}
	}

	// Create render targets
	if( !createRenderTargets() )
	{
		return raiseError( "Failed to create render target" );
	}

	return true;
}


void PipelineResource::resize( uint32 width, uint32 height )
{
	_baseWidth = width;
	_baseHeight = height;
	// Recreate render targets
	releaseRenderTargets();
	createRenderTargets();
}


int PipelineResource::getElemCount( int elem )
{
	switch( elem )
	{
	case PipelineResData::StageElem:
		return (int)_stages.size();
	default:
		return Resource::getElemCount( elem );
	}
}


int PipelineResource::getElemParamI( int elem, int elemIdx, int param )
{
	switch( elem )
	{
	case PipelineResData::StageElem:
		if( (unsigned)elemIdx < _stages.size() )
		{
			switch( param )
			{
			case PipelineResData::StageActivationI:
				return _stages[elemIdx]->enabled ? 1 : 0;
			}
		}
		break;
	}

	return Resource::getElemParamI( elem, elemIdx, param );
}


void PipelineResource::setElemParamStr(int elem, int elemIdx, int param, const char* value)
{
   switch( elem )
   {
   case PipelineResData::GlobalRenderTarget:
      RenderTarget* t = findRenderTarget(std::string(value));
      if (t != 0x0) {
         t->rendBuf = param;
      }
   }
}

void PipelineResource::setElemParamI( int elem, int elemIdx, int param, int value )
{
	switch( elem )
	{
	case PipelineResData::StageElem:
		if( (unsigned)elemIdx < _stages.size() )
		{
			switch( param )
			{
			case PipelineResData::StageActivationI:
				_stages[elemIdx]->enabled = (value == 0) ? 0 : 1;
				return;
			}
		}
		break;
	}

	Resource::setElemParamI( elem, elemIdx, param, value );
}


const char *PipelineResource::getElemParamStr( int elem, int elemIdx, int param )
{
	switch( elem )
	{
	case PipelineResData::StageElem:
		if( (unsigned)elemIdx < _stages.size() )
		{
			switch( param )
			{
			case PipelineResData::StageNameStr:
				return _stages[elemIdx]->id.c_str();
			}
		}
		break;
	}

	return Resource::getElemParamStr( elem, elemIdx, param );
}


bool PipelineResource::getRenderTargetData( std::string const& target, int bufIndex, int *width, int *height,
                                            int *compCount, void *dataBuffer, int bufferSize )
{
	uint32 rbObj = 0;
	if( !target.empty() )
	{	
		RenderTarget *rt = findRenderTarget( target );
		if( rt == 0x0 ) return false;
      else rbObj = rt->rendBuf;
	}
	
	return gRDI->getRenderBufferData(
		rbObj, bufIndex, width, height, compCount, dataBuffer, bufferSize );
}

}  // namespace
