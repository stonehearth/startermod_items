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

#include "radiant.h"
#include "radiant_logger.h"
#include "core/config.h"

#include "egCom.h"
#include "utMath.h"
#include "egModules.h"
#include "egRenderer.h"
#include <stdarg.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "utDebug.h"

namespace Horde3D {

// *************************************************************************************************
// Class EngineConfig
// *************************************************************************************************

EngineConfig::EngineConfig()
{
   // preserve the hold horde logging behavior, defaulting to 4.  but if the user overrides it,
   // use what's in their user_settings.json file!
   radiant::core::Config& config = radiant::core::Config::GetInstance();
   if (config.Get<int>("logging.horde.general", -999) == -999) {
      log_levels_.horde.general = 2;
   }

	trilinearFiltering = true;
	maxAnisotropy = 1;
	texCompression = false;
	sRGBLinearization = false;
	loadTextures = true;
	fastAnimation = true;
	shadowMapQuality = 1;
	sampleCount = 0;
	wireframeMode = false;
	debugViewMode = false;
	dumpFailedShaders = false;
	gatherTimeStats = true;
   enableShadows = true;
   overlayAspect = 1.0;
   enableStatsLogging = false;
   disablePinnedMemory = false;
   maxLights = 32;
   dumpCompiledShaders = false;
}


float EngineConfig::getOption( EngineOptions::List param )
{
   EngineRendererCaps rendererCaps;
   EngineGpuCaps gpuCaps;
   Modules::renderer().getEngineCapabilities(&rendererCaps, &gpuCaps);
	switch( param )
	{
	case EngineOptions::TrilinearFiltering:
		return trilinearFiltering ? 1.0f : 0.0f;
	case EngineOptions::MaxAnisotropy:
		return (float)maxAnisotropy;
	case EngineOptions::TexCompression:
		return texCompression ? 1.0f : 0.0f;
	case EngineOptions::SRGBLinearization:
		return sRGBLinearization ? 1.0f : 0.0f;
	case EngineOptions::LoadTextures:
		return loadTextures ? 1.0f : 0.0f;
	case EngineOptions::FastAnimation:
		return fastAnimation ? 1.0f : 0.0f;
	case EngineOptions::ShadowMapQuality:
		return (float)shadowMapQuality;
	case EngineOptions::SampleCount:
		return (float)sampleCount;
	case EngineOptions::WireframeMode:
		return wireframeMode ? 1.0f : 0.0f;
	case EngineOptions::DebugViewMode:
		return debugViewMode ? 1.0f : 0.0f;
	case EngineOptions::DumpFailedShaders:
		return dumpFailedShaders ? 1.0f : 0.0f;
	case EngineOptions::GatherTimeStats:
		return gatherTimeStats ? 1.0f : 0.0f;
   case EngineOptions::EnableShadows:
      return rendererCaps.ShadowsSupported && enableShadows ? 1.0f : 0.0f;
   case EngineOptions::EnableStatsLogging:
      return enableStatsLogging ? 1.0f : 0.0f;
   case EngineOptions::DisablePinnedMemory:
      return disablePinnedMemory ? 1.0f : 0.0f;
   case EngineOptions::MaxLights:
      return (float)maxLights;
	case EngineOptions::DumpCompiledShaders:
		return dumpCompiledShaders ? 1.0f : 0.0f;
	default:
		Modules::setError( "Invalid param for h3dGetOption" );
		return Math::NaN;
	}
}


bool EngineConfig::setOption( EngineOptions::List param, float value )
{
	int size;
   EngineRendererCaps rendererCaps;
   EngineGpuCaps gpuCaps;
   Modules::renderer().getEngineCapabilities(&rendererCaps, &gpuCaps);
	
	switch( param )
	{
	case EngineOptions::TrilinearFiltering:
		trilinearFiltering = (value != 0);
		return true;
	case EngineOptions::MaxAnisotropy:
		maxAnisotropy = ftoi_r( value );
		return true;
	case EngineOptions::TexCompression:
		texCompression = (value != 0);
		return true;
	case EngineOptions::SRGBLinearization:
		sRGBLinearization = (value != 0);
		return true;
	case EngineOptions::LoadTextures:
		loadTextures = (value != 0);
		return true;
	case EngineOptions::FastAnimation:
		fastAnimation = (value != 0);
		return true;
   case EngineOptions::MaxLights:
      maxLights = (int)value;
      return true;
	case EngineOptions::ShadowMapQuality:
      if (!enableShadows || !rendererCaps.ShadowsSupported) {
         return true;
      }
		size = ftoi_r(value);

		if (size == shadowMapQuality) {
         return true;
      }

      shadowMapQuality = size;

		// Update shadow maps
      Modules::renderer().reallocateShadowBuffers(shadowMapQuality);
		
		return true;
	case EngineOptions::SampleCount:
      sampleCount = gRDI->getCaps().rtMultisampling ? ftoi_r( value ) : 0;
		return true;
	case EngineOptions::WireframeMode:
		wireframeMode = (value != 0);
		return true;
	case EngineOptions::DebugViewMode:
		debugViewMode = (value != 0);
		return true;
	case EngineOptions::DumpFailedShaders:
		dumpFailedShaders = (value != 0);
		return true;
	case EngineOptions::GatherTimeStats:
		gatherTimeStats = (value != 0);
		return true;
   case EngineOptions::EnableShadows:
      if (!rendererCaps.ShadowsSupported) {
         return true;
      }
      enableShadows = (value != 0);
      Modules::renderer().reallocateShadowBuffers(shadowMapQuality);
      setGlobalShaderFlag("DISABLE_SHADOWS", !enableShadows);
      return true;
   case EngineOptions::EnableStatsLogging:
      enableStatsLogging = (value != 0);
      return true;
   case EngineOptions::DisablePinnedMemory:
      disablePinnedMemory = (value != 0);
      return true;
	case EngineOptions::DumpCompiledShaders:
		dumpCompiledShaders = (value != 0);
		return true;
	default:
		Modules::setError( "Invalid param for h3dSetOption" );
		return false;
	}
}

void EngineConfig::setGlobalShaderFlag(std::string const& name, bool value)
{

   if (shaderFlags.find(name) != shaderFlags.end()) {
      if (!value) {
         shaderFlags.erase(name);
      }
      return;
   }

   if (!value) {
      return;
   }

   shaderFlags.insert(name);
}

bool EngineConfig::isGlobalShaderFlagSet(std::string const& name)
{

   if (shaderFlags.find(name) != shaderFlags.end()) {
      return true;
   }

   return false;
}


// *************************************************************************************************
// Class EngineLog
// *************************************************************************************************

EngineLog::EngineLog()
{
}


void EngineLog::pushMessage( int level, const char *msg, va_list args )
{
#if defined( PLATFORM_WIN )
#pragma warning( push )
#pragma warning( disable:4996 )
	vsnprintf( _textBuf, ARRAY_SIZE(_textBuf), msg, args );
#pragma warning( pop )
#else
	vsnprintf( _textBuf, ARRAY_SIZE(_textBuf), msg, args );
#endif
   LOG(horde.general, (unsigned int)level) << _textBuf;
}


void EngineLog::writeError( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );
	pushMessage( 1, msg, args );
	va_end( args );
}


void EngineLog::writeWarning( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );
	pushMessage( 2, msg, args );
	va_end( args );
}


void EngineLog::writeInfo( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );
	pushMessage( 3, msg, args );
	va_end( args );
}


void EngineLog::writeDebugInfo( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );
	pushMessage( 4, msg, args );
	va_end( args );
}

void EngineLog::writePerfInfo(const char *msg, ...)
{
   if(!Modules::config().enableStatsLogging) {
      return;
   }

	va_list args;
	va_start( args, msg );
	pushMessage( 5, msg, args );
	va_end( args );
}

void EngineLog::SetNotifyErrorCb(ReportErrorCb const& cb)
{
   error_cb_ = cb;
   for (auto const& record : errors_) {
      cb(record);
   }
   errors_.clear();
}

void EngineLog::ReportError(::radiant::om::ErrorBrowser::Record const& record)
{
   if (error_cb_) {
      error_cb_(record);
   } else {
      errors_.emplace_back(record);
   }
}


// *************************************************************************************************
// Class StatManager
// *************************************************************************************************

StatManager::StatManager()
{
	_statTriCount = 0;
	_statBatchCount = 0;
	_statLightPassCount = 0;
   _statShadowPassCount = 0;

   _curFrame = 0;
	_frameTime = 0;
   _totalFrames = 0;
	for (int i = 0; i < ARRAY_SIZE(_frameTimes); i++)
	{
		_frameTimes[i] = 0.0;
	}
}


StatManager::~StatManager()
{
}


float StatManager::getStat( int param, bool reset )
{
	float value;	
   float sum = 0.0;
	float availableMem = 0;
   int integerV[4];
   int c;

	switch( param )
	{
	case EngineStats::TriCount:
		value = (float)_statTriCount;
		if( reset ) _statTriCount = 0;
		return value;
	case EngineStats::BatchCount:
		value = (float)_statBatchCount;
		if( reset ) _statBatchCount = 0;
		return value;
	case EngineStats::LightPassCount:
		value = (float)_statLightPassCount;
		if( reset ) _statLightPassCount = 0;
		return value;
	case EngineStats::ShadowPassCount:
		value = (float)_statShadowPassCount;
		if( reset ) _statShadowPassCount = 0;
		return value;
	case EngineStats::FrameTime:
		value = _frameTime;
		if( reset ) _frameTime = 0;
		return value;
   case EngineStats::AverageFrameTime:
      c = std::min((int)_totalFrames, (int)ARRAY_SIZE(_frameTimes));
      for (int i = 0; i < c; i++)
      {
         sum += _frameTimes[i];
      }
      value = sum / c;
      return value;
	case EngineStats::AnimationTime:
      value = (float)radiant::perfmon::CounterToMilliseconds(_animTimer.GetElapsed());
      if( reset ) _animTimer.Reset();
		return value;
	case EngineStats::GeoUpdateTime:
      value = (float)radiant::perfmon::CounterToMilliseconds(_geoUpdateTimer.GetElapsed());
		if( reset ) _geoUpdateTimer.Reset();
		return value;
	case EngineStats::ParticleSimTime:
      value = (float)radiant::perfmon::CounterToMilliseconds(_particleSimTimer.GetElapsed());
		if( reset ) _particleSimTimer.Reset();
		return value;
	case EngineStats::TextureVMem:
		return (gRDI->getTextureMem() / 1024) / 1024.0f;
	case EngineStats::GeometryVMem:
		return (gRDI->getBufferMem() / 1024) / 1024.0f;
   case EngineStats::AvailableGpuMemory:
      if (gRDI->getCaps().cardType == NVIDIA) {
         glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, integerV);
         availableMem = integerV[0] / 1024.0f;
      } else if (gRDI->getCaps().cardType == ATI) {
         glGetIntegerv(VBO_FREE_MEMORY_ATI, integerV);
         availableMem = integerV[0] / 1024.0f;
      }
      glGetError();
      return availableMem;
	default:
		Modules::setError( "Invalid param for h3dGetStat" );
		return Math::NaN;
	}
}


void StatManager::incStat( int param, float value )
{
	switch( param )
	{
	case EngineStats::TriCount:
		_statTriCount += ftoi_r( value );
		break;
	case EngineStats::BatchCount:
		_statBatchCount += ftoi_r( value );
		break;
	case EngineStats::LightPassCount:
		_statLightPassCount += ftoi_r( value );
		break;
	case EngineStats::ShadowPassCount:
		_statShadowPassCount += ftoi_r( value );
		break;
	case EngineStats::FrameTime:
		_frameTime += value;
      _frameTimes[_curFrame] = _frameTime;
      _totalFrames++;
      _curFrame = (_curFrame + 1) % ARRAY_SIZE(_frameTimes);
		break;
	}
}


radiant::perfmon::Timer *StatManager::getTimer( int param )
{
	switch( param )
	{
	case EngineStats::FrameTime:
		return &_frameTimer;
	case EngineStats::AnimationTime:
		return &_animTimer;
	case EngineStats::GeoUpdateTime:
		return &_geoUpdateTimer;
	case EngineStats::ParticleSimTime:
		return &_particleSimTimer;
	default:
		return 0x0;
	}
}

}  // namespace
