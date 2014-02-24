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

#include "egInstanceNode.h"
#include "egMaterial.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"

#include "utDebug.h"


namespace Horde3D {

using namespace std;

// *************************************************************************************************
// InstanceNode
// *************************************************************************************************

InstanceNode::InstanceNode( const InstanceNodeTpl &nodeTpl ) :
	SceneNode( nodeTpl )
{
	_renderable = true;
   _geoRes = nodeTpl._geoRes;
   _maxInstances = nodeTpl._maxInstances;
   _usedInstances = 0;
   _matRes = nodeTpl._matRes;

   _instanceBuf = new float[16 * _maxInstances];
   _instanceBufObj = gRDI->createVertexBuffer(_maxInstances * sizeof(float) * 16, RDIBufferUsage::STREAM, 0x0);
}


InstanceNode::~InstanceNode()
{
   if (_instanceBuf) {
      delete[] _instanceBuf;
      _instanceBuf = 0x0;
   }

   if (_instanceBufObj) {
      gRDI->destroyBuffer(_instanceBufObj);
      _instanceBufObj = 0;
   }

   _geoRes = 0x0;
}


SceneNodeTpl *InstanceNode::parsingFunc( map< string, std::string > &attribs )
{
	InstanceNodeTpl *emitterTpl = new InstanceNodeTpl("", 0x0, 0x0, 0);
	return emitterTpl;
}


SceneNode *InstanceNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SceneNodeTypes::InstanceNode ) return 0x0;
	return new InstanceNode( *(InstanceNodeTpl *)&nodeTpl );
}


int InstanceNode::getParamI( int param )
{
   return SceneNode::getParamI(param);
}


void InstanceNode::setParamI( int param, int value )
{
	SceneNode::setParamI( param, value );
}


float InstanceNode::getParamF( int param, int compIdx )
{
	return SceneNode::getParamF( param, compIdx );
}


void InstanceNode::setParamF( int param, int compIdx, float value )
{
	SceneNode::setParamF( param, compIdx, value );
}

void* InstanceNode::mapParamV(int param)
{
   switch(param) {
   case InstanceNodeParams::InstanceBuffer:
      return _instanceBuf;
      break;
   }
	return SceneNode::mapParamV( param );
}

void InstanceNode::unmapParamV(int param, int mappedLength)
{
   switch(param) {
   case InstanceNodeParams::InstanceBuffer:
      _usedInstances = mappedLength / (sizeof(float) * 16);
      gRDI->updateBufferData(_instanceBufObj, 0, mappedLength, _instanceBuf);
      return;
      break;
   }
	SceneNode::unmapParamV( param, mappedLength );
}

void InstanceNode::onPostUpdate()
{	

}

}  // namespace
