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

#include "utils.h"
#include "utPlatform.h"
#include <iostream>

#ifdef PLATFORM_WIN
#   define WIN32_LEAN_AND_MEAN 1
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <direct.h>
#else
#	include <sys/stat.h>  // For mkdir
#endif

using namespace std;


void removeGate( std::string &s )
{
	if( s.length() == 0 ) return;

	if( s[0] == '#' ) s = s.substr( 1, s.length() - 1 );
}


std::string decodeURL( const std::string &url )
{
	std::string buf = "";

	for( size_t i = 0, len = url.length(); i < len; ++i )
	{
		if( url[i] == '%' )
		{
			char code[] = { '0', 'x', '0', url.at( i + 1 ), url.at( i + 2 ) };
			buf += (char)strtol( code, 0x0, 16 );
			i += 2;
		}
		else
		{
			buf += url[i];
		}
	}
	
	return buf;
}


std::string extractFileName( const std::string &fullPath, bool extension )
{
	int first = 0, last = (int)fullPath.length() - 1;
	
	for( int i = last; i >= 0; --i )
	{
		if( fullPath[i] == '.' )
		{
			last = i;
		}
		else if( fullPath[i] == '\\' || fullPath[i] == '/' )
		{
			first = i + 1;
			break;
		}
	}

	if( extension )
		return fullPath.substr( first, fullPath.length() - first );
	else
		return fullPath.substr( first, last - first );
}


std::string extractFilePath( const std::string &fullPath )
{
	int last = 0;
	
	for( int i = (int)fullPath.length() - 1; i >= 0; --i )
	{
		if( fullPath[i] == '\\' || fullPath[i] == '/' )
		{
			last = i;
			break;
		}
	}

	return fullPath.substr( 0, last );
}


std::string cleanPath( const std::string &path )
{
	size_t len = path.length();
	if( len == 0 ) return path;
	
	if( path[len - 1] == '/' || path[len - 1] == '\\' )
		return path.substr( 0, len - 1 );
	else
		return path;
}


void log( const std::string &msg )
{
	cout << msg << endl;
	
#ifdef PLATFORM_WIN
	OutputDebugString( msg.c_str() );
	OutputDebugString( "\r\n" );
#endif
}


Matrix4f makeMatrix4f( float *floatArray16, bool y_up )
{
	Matrix4f mat( floatArray16 );
	mat = mat.transposed();		// Expects floatArray16 to be row-major

	// Flip matrix if necessary
	if( !y_up )
	{
		// Swap y/z rows
		swap( mat.c[0][1], mat.c[0][2] );
		swap( mat.c[1][1], mat.c[1][2] );
		swap( mat.c[2][1], mat.c[2][2] );
		swap( mat.c[3][1], mat.c[3][2] );

		// Swap y/z columns
		swap( mat.c[1][0], mat.c[2][0] );
		swap( mat.c[1][1], mat.c[2][1] );
		swap( mat.c[1][2], mat.c[2][2] );
		swap( mat.c[1][3], mat.c[2][3] );

		// Invert z-axis to make system right-handed again
		// (The swapping above results in a left-handed system)
		mat.c[0][2] *= -1;
		mat.c[1][2] *= -1;
		mat.c[3][2] *= -1;
		mat.c[2][0] *= -1;
		mat.c[2][1] *= -1;
		mat.c[2][3] *= -1;
	}

	return mat;
}


void createDirectories( const std::string &basePath, const std::string &newPath )
{
	if( newPath.empty() ) return;
	
	std::string tmpString;
	tmpString.reserve( 256 );
	size_t i = 0, len = newPath.length();

	while( ++i < len )
	{
		if( newPath[i] == '/' || newPath[i] == '\\' || i == len-1 )
		{
			tmpString = basePath + newPath.substr( 0, ++i );
			_mkdir( tmpString.c_str() );
		}
	}
}
