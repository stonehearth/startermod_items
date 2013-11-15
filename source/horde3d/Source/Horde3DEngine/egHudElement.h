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

#ifndef _egHudElement_H_
#define _egHudElement_H_

#include "egPrerequisites.h"
#include "egScene.h"
#include "egMaterial.h"
#include "egGeometry.h"
#include "utMath.h"


namespace Horde3D {

class HudElement
{
public:
   const BoundingBox& getBounds() const;
   virtual void updateGeometry(const Matrix4f& absTrans) = 0;
   virtual void draw(const std::string &shaderContext, const std::string &theClass) {}

protected:
   BoundingBox bounds_;
};

class RectHudElement : public HudElement
{
public:

   void updateGeometry(const Matrix4f& absTrans);
   void draw(const std::string &shaderContext, const std::string &theClass);

   RectHudElement* setSize(int width, int height);
   RectHudElement* setOffsets(int offsetX, int offsetY);

private:
   PMaterialResource materialRes_;

   int width_, height_;
   int offsetX_, offsetY_;

   uint32 rectVBO_;
   uint32 rectIdxBuf_;
   uint32 vlRect_;

protected:
   RectHudElement(int width, int height, int offsetX, int offsetY, ResHandle matRes);
   ~RectHudElement();

   friend class HudElementNode;
};



// =================================================================================================
// HudElement Node
// =================================================================================================

struct HudElementNodeParams
{
	enum List
	{
	};
};

// =================================================================================================

struct HudElementNodeTpl : public SceneNodeTpl
{

	HudElementNodeTpl( const std::string &name ) :
		SceneNodeTpl( SceneNodeTypes::HudElement, name )
	{
	}
};

// =================================================================================================

class HudElementNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	~HudElementNode();

   RectHudElement* addRect(int width, int height, int offsetX, int offsetY, ResHandle matRes);

	void recreateNodeList();
	int getParamI( int param );
	void setParamI( int param, int value );
	float getParamF( int param, int compIdx );
	void setParamF( int param, int compIdx, float value );

	bool updateGeometry();

   const std::vector<HudElement*>& getSubElements() const;

protected:
	HudElementNode( const HudElementNodeTpl &modelTpl );

	void onPostUpdate();
	void onFinishedUpdate();

private:
   std::vector<HudElement*>   elements_;


protected:
	friend class SceneManager;
	friend class SceneNode;
	friend class Renderer;
};

}
#endif // _egHudElement_H_
