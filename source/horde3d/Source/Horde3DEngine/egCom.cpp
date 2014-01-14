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
	maxLogLevel = 4;
	trilinearFiltering = true;
	maxAnisotropy = 1;
	texCompression = false;
	sRGBLinearization = false;
	loadTextures = true;
	fastAnimation = true;
	shadowMapSize = 1024;
	sampleCount = 0;
	wireframeMode = false;
	debugViewMode = false;
	dumpFailedShaders = false;
	gatherTimeStats = true;
   enableShadows = true;
   overlayAspect = 1.0;
}


float EngineConfig::getOption( EngineOptions::List param )
{
	switch( param )
	{
	case EngineOptions::MaxLogLevel:
		return (float)maxLogLevel;
	case EngineOptions::MaxNumMessages:
		return (float)Modules::log().getMaxNumMessages();
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
	case EngineOptions::ShadowMapSize:
		return (float)shadowMapSize;
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
      return enableShadows ? 1.0f : 0.0f;
	default:
		Modules::setError( "Invalid param for h3dGetOption" );
		return Math::NaN;
	}
}


bool EngineConfig::setOption( EngineOptions::List param, float value )
{
	int size;
	
	switch( param )
	{
	case EngineOptions::MaxLogLevel:
		maxLogLevel = ftoi_r( value );
		return true;
	case EngineOptions::MaxNumMessages:
		Modules::log().setMaxNumMessages( (uint32)ftoi_r( value ) );
		return true;
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
	case EngineOptions::ShadowMapSize:
		size = ftoi_r( value );

		if( size == shadowMapSize ) return true;

      if ( size <= 512 ) {
         size = 512;
      }

      size = (int)pow(2, floor(log(size) / log(2.0)));

      size = std::min(size, gRDI->getCaps().maxTextureSize);

		// Update shadow map
		Modules::renderer().releaseShadowRB();
		
		if( !Modules::renderer().createShadowRB( size, size ) )
		{
			Modules::log().writeError( "Failed to create shadow map" );
			// Restore old buffer
			Modules::renderer().createShadowRB( shadowMapSize, shadowMapSize );
			return false;
		}
		else
		{
			shadowMapSize = size;
			return true;
		}
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
      enableShadows = (value != 0);
      setGlobalShaderFlag("DISABLE_SHADOWS", !enableShadows);
      return true;
	default:
		Modules::setError( "Invalid param for h3dSetOption" );
		return false;
	}
}

void EngineConfig::setGlobalShaderFlag(const char* name, bool value)
{
   std::string flag(name);

   if (shaderFlags.find(flag) != shaderFlags.end()) {
      if (!value) {
         shaderFlags.erase(flag);
      }
      return;
   }

   if (!value) {
      return;
   }

   shaderFlags.insert(flag);
}

bool EngineConfig::isGlobalShaderFlagSet(const char* name)
{
   std::string flag(name);

   if (shaderFlags.find(flag) != shaderFlags.end()) {
      return true;
   }

   return false;
}


// *************************************************************************************************
// Class EngineLog
// *************************************************************************************************


void EngineLog::dumpMessages()
{
   if (!_outf.is_open()) {
      return;
   }

   while (!_messages.empty())
   {
      const LogMessage& message = _messages.front();
      _outf << "[";
      _outf << std::fixed << std::setw(9) << std::setfill('0') << message.time;
      _outf << " ";
		
      switch(message.level)
      {
      case 1:
         _outf << "ERR]";
         break;
      case 2:
         _outf << "WRN]";
         break;
      case 3:
         _outf << "INF]";
      break;
         default:
         _outf << "DBG]";
      }
		
      _outf << " ";
      _outf << message.text.c_str();
      _outf << "\n";
      _messages.pop();
   }
   _outf.flush();	
}

EngineLog::EngineLog(const std::string& logFilePath)
{
	_timer.setEnabled( true );
	_maxNumMessages = 512;
	// Reset log file
	_outf.setf( std::ios::fixed );
	_outf.precision( 3 );

	_outf.open(logFilePath, std::ios::out );	
}


void EngineLog::pushMessage( int level, const char *msg, va_list args )
{
	float time = _timer.getElapsedTimeMS() / 1000.0f;

#if defined( PLATFORM_WIN )
#pragma warning( push )
#pragma warning( disable:4996 )
	vsnprintf( _textBuf, 2048, msg, args );
#pragma warning( pop )
#else
	vsnprintf( _textBuf, 2048, msg, args );
#endif
	
	if( _messages.size() < _maxNumMessages - 1 )
	{
		_messages.push( LogMessage( _textBuf, level, time ) );
	}
	else if( _messages.size() == _maxNumMessages - 1 )
	{
		_messages.push( LogMessage( "Message queue is full", 1, time ) );
	}

   dumpMessages();

#if defined( PLATFORM_WIN ) && defined( H3D_DEBUGGER_OUTPUT )
	const TCHAR *headers[6] = { TEXT(""), TEXT("  [h3d-err] "), TEXT("  [h3d-warn] "), TEXT("[h3d] "), TEXT("  [h3d-dbg] "), TEXT("[h3d- ] ")};
	
	OutputDebugString( headers[std::min( (uint32)level, (uint32)5 )] );
	OutputDebugStringA( _textBuf );
	OutputDebugString( TEXT("\r\n") );
#endif
}


void EngineLog::writeError( const char *msg, ... )
{
	if( Modules::config().maxLogLevel < 1 ) return;

	va_list args;
	va_start( args, msg );
	pushMessage( 1, msg, args );
	va_end( args );
}


void EngineLog::writeWarning( const char *msg, ... )
{
	if( Modules::config().maxLogLevel < 2 ) return;

	va_list args;
	va_start( args, msg );
	pushMessage( 2, msg, args );
	va_end( args );
}


void EngineLog::writeInfo( const char *msg, ... )
{
	if( Modules::config().maxLogLevel < 3 ) return;

	va_list args;
	va_start( args, msg );
	pushMessage( 3, msg, args );
	va_end( args );
}


void EngineLog::writeDebugInfo( const char *msg, ... )
{
	if( Modules::config().maxLogLevel < 4 ) return;

	va_list args;
	va_start( args, msg );
	pushMessage( 4, msg, args );
	va_end( args );
}


bool EngineLog::getMessage( LogMessage &msg )
{
	if( !_messages.empty() )
	{
		msg = _messages.front();
		_messages.pop();
		return true;
	}
	else
		return false;
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
		value = _animTimer.getElapsedTimeMS();
		if( reset ) _animTimer.reset();
		return value;
	case EngineStats::GeoUpdateTime:
		value = _geoUpdateTimer.getElapsedTimeMS();
		if( reset ) _geoUpdateTimer.reset();
		return value;
	case EngineStats::ParticleSimTime:
		value = _particleSimTimer.getElapsedTimeMS();
		if( reset ) _particleSimTimer.reset();
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
	case EngineStats::FrameTime:
		_frameTime += value;
      _frameTimes[_curFrame] = _frameTime;
      _totalFrames++;
      _curFrame = (_curFrame + 1) % ARRAY_SIZE(_frameTimes);
		break;
	}
}


Timer *StatManager::getTimer( int param )
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
