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

#ifndef _egInstanceNode_H_
#define _egInstanceNode_H_

#include "egPrerequisites.h"
#include "utMath.h"
#include "egMaterial.h"
#include "egVoxelGeometry.h"
#include "egScene.h"


namespace Horde3D {

struct InstanceNodeParams
{
	enum List
	{
      InstanceBuffer
	};
};


// =================================================================================================

struct InstanceNodeTpl : public SceneNodeTpl
{
   InstanceNodeTpl( const std::string &name, MaterialResource* matRes, VoxelGeometryResource* geoRes, int maxInstances ) :
      SceneNodeTpl( SceneNodeTypes::InstanceNode, name ), _geoRes(geoRes), _matRes(matRes), _maxInstances(maxInstances)
	{
	}

   int _maxInstances;
   PVoxelGeometryResource _geoRes;
   PMaterialResource _matRes;
};

// =================================================================================================

class InstanceNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	~InstanceNode();

	int getParamI( int param );
	void setParamI( int param, int value );
	float getParamF( int param, int compIdx );
	void setParamF( int param, int compIdx, float value );
   void* mapParamV(int param);
   void unmapParamV(int param, int mappedLength);

protected:
	InstanceNode( const InstanceNodeTpl &emitterTpl );

	void onPostUpdate();

protected:

   int _maxInstances;
   int _usedInstances;
   PVoxelGeometryResource _geoRes;
   PMaterialResource _matRes;
	
   float* _instanceBuf;
   uint32 _instanceBufObj;

   friend class SceneManager;
	friend class Renderer;
};

}
#endif // _egInstanceNode_H_
