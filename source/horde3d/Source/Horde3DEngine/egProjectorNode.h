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

#ifndef _egProjectorNode_H_
#define _egProjectorNode_H_

#include "egPrerequisites.h"
#include "utMath.h"
#include "egMaterial.h"
#include "egScene.h"


namespace Horde3D {

struct ProjectorNodeParams
{
	enum List
	{
		MatResI = 300
	};
};


// =================================================================================================

struct ProjectorNodeTpl : public SceneNodeTpl
{
   ProjectorNodeTpl( const std::string &name, MaterialResource* matRes ) :
      SceneNodeTpl( SceneNodeTypes::ProjectorNode, name ), _matRes(matRes)
	{
	}

   PMaterialResource _matRes;
};

// =================================================================================================

class ProjectorNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	~ProjectorNode();

	int getParamI( int param );
	void setParamI( int param, int value );
	float getParamF( int param, int compIdx );
	void setParamF( int param, int compIdx, float value );

   MaterialResource* getMaterialRes();

protected:
	ProjectorNode( const ProjectorNodeTpl &projectorTpl );

private:

   PMaterialResource _matRes;
	
   friend class SceneManager;
	friend class Renderer;
};

}
#endif // _egProjectorNode_H_
