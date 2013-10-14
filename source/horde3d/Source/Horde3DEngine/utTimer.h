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

#ifndef _utTimer_H_
#define _utTimer_H_

#include "utPlatform.h"

#if defined( PLATFORM_WIN ) || defined( PLATFORM_WIN_CE )
#  if !defined(WIN32_LEAN_AND_MEAN)
#     define WIN32_LEAN_AND_MEAN 1
#  endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#   include <windows.h>
#else
#	include <sys/time.h>
#endif


namespace Horde3D {

class Timer
{
public:

	Timer() : _elapsedTime( 0 ), _enabled( false )
	{
	#if defined( PLATFORM_WIN ) || defined( PLATFORM_WIN_CE)
		QueryPerformanceFrequency( &_timerFreq );
	#endif
	}
	
	void setEnabled( bool enabled )
	{	
		if( enabled && !_enabled )
		{
			_startTime = getTime();
			_enabled = true;
		}
		else if( !enabled && _enabled )
		{
			double endTime = getTime();
			_elapsedTime += endTime - _startTime;
			_enabled = false;
		}
	}

	void reset()
	{
		_elapsedTime = 0;
		if( _enabled ) _startTime = getTime();
	}
	
	float getElapsedTimeMS()
	{
		if( _enabled )
		{
			double endTime = getTime();
			_elapsedTime += endTime - _startTime;
			_startTime = endTime;
		}

		return (float)_elapsedTime;
	}

protected:

	double getTime()
	{
	#if defined( PLATFORM_WIN ) || defined( PLATFORM_WIN_CE )
		LARGE_INTEGER curTick;

		// QueryPerformanceCounter may not behave correctly if the user has a buggy
		// BIOS and we call don't call it on the same core everytime.  The penalty
		// for ensuring that we're on the same core everytime, though, is just too
		// high (it would require a forced context switch inside getTime).  If we
		// are really concerned, the proper fix is to set the render thread's
		// affinity at horde init time and verify we're on that core at the time
		// getTime() is called (in cause the client also set the affinity mask to
		// something else).
		QueryPerformanceCounter( &curTick );

		return (double)curTick.QuadPart / (double)_timerFreq.QuadPart * 1000.0;
	#else
		timeval tv;
		gettimeofday( &tv, 0x0 );
		return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
	#endif
	}

protected:

	double         _startTime;
	double         _elapsedTime;

#if defined( PLATFORM_WIN ) || defined( PLATFORM_WIN_CE )
	LARGE_INTEGER  _timerFreq;
#endif

	bool           _enabled;
};

}
#endif  // _utTimer_H_
