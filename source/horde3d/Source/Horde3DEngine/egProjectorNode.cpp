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

#include "egProjectorNode.h"
#include "egMaterial.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"

#include "utDebug.h"


namespace Horde3D {

using namespace std;

// *************************************************************************************************
// ProjectorNode
// *************************************************************************************************

ProjectorNode::ProjectorNode( const ProjectorNodeTpl &nodeTpl ) :
	SceneNode( nodeTpl )
{
   _matRes = nodeTpl._matRes;
}


ProjectorNode::~ProjectorNode()
{
   _matRes = 0x0;
}

PMaterialResource& ProjectorNode::getMaterialRes()
{
   return _matRes;
}


SceneNodeTpl *ProjectorNode::parsingFunc( map< string, std::string > &attribs )
{
	ProjectorNodeTpl *projectorTpl = new ProjectorNodeTpl("", 0x0);
	return projectorTpl;
}


SceneNode *ProjectorNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SceneNodeTypes::ProjectorNode ) return 0x0;
	return new ProjectorNode( *(ProjectorNodeTpl *)&nodeTpl );
}


int ProjectorNode::getParamI( int param )
{
   return SceneNode::getParamI(param);
}


void ProjectorNode::setParamI( int param, int value )
{
	SceneNode::setParamI( param, value );
}


float ProjectorNode::getParamF( int param, int compIdx )
{
	return SceneNode::getParamF( param, compIdx );
}


void ProjectorNode::setParamF( int param, int compIdx, float value )
{
	SceneNode::setParamF( param, compIdx, value );
}

}  // namespace
