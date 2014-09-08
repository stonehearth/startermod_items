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

#ifndef _egCom_H_
#define _egCom_H_

#include "egPrerequisites.h"
#include <string>
#include <set>
#include <queue>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <functional>
#include "utTimer.h"
#include "om/error_browser/error_browser.h"

namespace Horde3D {

// =================================================================================================
// Engine Config
// =================================================================================================

struct EngineOptions
{
	enum List
	{
		MaxLogLevel = 1,
		MaxNumMessages,
		TrilinearFiltering,
		MaxAnisotropy,
		TexCompression,
		SRGBLinearization,
		LoadTextures,
		FastAnimation,
		ShadowMapSize,
		SampleCount,
		WireframeMode,
		DebugViewMode,
		DumpFailedShaders,
		GatherTimeStats,
      EnableShadows,
      EnableStatsLogging,
      DisablePinnedMemory
	};
};

// =================================================================================================

class EngineConfig
{
public:
	EngineConfig();

	float getOption( EngineOptions::List param );
	bool setOption( EngineOptions::List param, float value );
   void setGlobalShaderFlag(const char* name, bool value);
   bool isGlobalShaderFlagSet(const char* name);

public:
	int   maxLogLevel;
	int   maxAnisotropy;
	int   shadowMapSize;
	int   sampleCount;
	bool  texCompression;
	bool  sRGBLinearization;
	bool  loadTextures;
	bool  fastAnimation;
	bool  trilinearFiltering;
	bool  wireframeMode;
	bool  debugViewMode;
	bool  dumpFailedShaders;
	bool  gatherTimeStats;
   bool  enableShadows;
   float overlayAspect;
   bool  enableStatsLogging;
   bool  disablePinnedMemory;
   std::set<std::string> shaderFlags;
};


// =================================================================================================
// Engine Log
// =================================================================================================

struct LogMessage
{
	std::string  text;
	int          level;
	float        time;

	LogMessage()
	{
	}

	LogMessage( std::string const& text, int level, float time ) :
		text( text ), level( level ), time( time )
	{
	}
};

// =================================================================================================

class EngineLog
{
public:
	EngineLog(std::string const& logFilePath);

	void writeError( const char *msg, ... );
	void writeWarning( const char *msg, ... );
	void writeInfo( const char *msg, ... );
	void writeDebugInfo( const char *msg, ... );
   void writePerfInfo(const char *msg, ...);

   void dumpMessages();

	bool getMessage( LogMessage &msg );

	uint32 getMaxNumMessages() { return _maxNumMessages; }
	void setMaxNumMessages( uint32 maxNumMessages ) { _maxNumMessages = maxNumMessages; }
	
   typedef std::function<void(::radiant::om::ErrorBrowser::Record const&)> ReportErrorCb;
   void SetNotifyErrorCb(ReportErrorCb const& cb);
   void ReportError(::radiant::om::ErrorBrowser::Record const&);

protected:
	void pushMessage( std::string const& text, uint32 level );
	void pushMessage( int level, const char *msg, va_list ap );

protected:
	Timer                     _timer;
	char                      _textBuf[2048];
	uint32                    _maxNumMessages;
	std::queue< LogMessage >  _messages;
   std::ofstream             _outf;
   ReportErrorCb             error_cb_;
   std::vector<::radiant::om::ErrorBrowser::Record> errors_;

};


// =================================================================================================
// Engine Stats
// =================================================================================================

struct EngineStats
{
	enum List
	{
		TriCount = 100,
		BatchCount,
		LightPassCount,
		FrameTime,
		AnimationTime,
		GeoUpdateTime,
		ParticleSimTime,
		TextureVMem,
		GeometryVMem,
      AverageFrameTime,
      AvailableGpuMemory
	};
};

// =================================================================================================

class StatManager
{
public:
	StatManager();
	~StatManager();
	
	float getStat( int param, bool reset );
	void incStat( int param, float value );
	Timer *getTimer( int param );

protected:
	uint32    _statTriCount;
	uint32    _statBatchCount;
	uint32    _statLightPassCount;

	Timer     _frameTimer;
	Timer     _animTimer;
	Timer     _geoUpdateTimer;
	Timer     _particleSimTimer;
	float     _frameTime;
   float     _frameTimes[100];
   int       _curFrame;
   int       _totalFrames;

	friend class ProfSample;
};

}
#endif // _egCom_H_
