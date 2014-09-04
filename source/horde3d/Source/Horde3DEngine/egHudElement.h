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
   virtual ~HudElement() {}
   const BoundingBox& getBounds() const;
   virtual void updateGeometry(const Matrix4f& absTrans) = 0;
   virtual void draw(std::string const& shaderContext, std::string const& theClass, Matrix4f& worldMat);

protected:

   BoundingBox bounds_;
};

class WorldspaceLineHudElement : public HudElement
{
public:
   void updateGeometry(const Matrix4f& absTrans);
   void draw(std::string const& shaderContext, std::string const& theClass, Matrix4f& worldMat);

private:
   PMaterialResource materialRes_;

   NodeHandle parentNode_;
   int width_;
   Vec4f color_;

   uint32 rectVBO_;
   uint32 rectIdxBuf_;

protected:
   WorldspaceLineHudElement(NodeHandle parentNode, int width, Vec4f color, ResHandle matRes);
   virtual ~WorldspaceLineHudElement() {}

   friend class HudElementNode;
};

class ScreenspaceRectHudElement : public HudElement
{
public:

   void updateGeometry(const Matrix4f& absTrans);
   void draw(std::string const& shaderContext, std::string const& theClass, Matrix4f& worldMat);

   ScreenspaceRectHudElement* setSize(int width, int height);
   ScreenspaceRectHudElement* setOffsets(int offsetX, int offsetY);

private:
   PMaterialResource materialRes_;

   float width_, height_;
   float offsetX_, offsetY_;
   Vec4f color_;

   uint32 rectVBO_;
   uint32 rectIdxBuf_;

protected:
   ScreenspaceRectHudElement(int width, int height, int offsetX, int offsetY, Vec4f color, ResHandle matRes);
   virtual ~ScreenspaceRectHudElement();

   friend class HudElementNode;
};


class WorldspaceRectHudElement : public HudElement
{
public:

   void updateGeometry(const Matrix4f& absTrans);
   void draw(std::string const& shaderContext, std::string const& theClass, Matrix4f& worldMat);

   WorldspaceRectHudElement* setSize(int width, int height);
   WorldspaceRectHudElement* setOffsets(int offsetX, int offsetY);

private:
   PMaterialResource materialRes_;

   float width_, height_;
   float offsetX_, offsetY_;
   Vec4f color_;

   uint32 rectVBO_;
   uint32 rectIdxBuf_;
   uint32 vlRect_;

protected:
   WorldspaceRectHudElement(float width, float height, float offsetX, float offsetY, Vec4f color, ResHandle matRes);
   virtual ~WorldspaceRectHudElement();

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

	HudElementNodeTpl( std::string const& name ) :
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

	virtual ~HudElementNode();

   
   ScreenspaceRectHudElement* addScreenspaceRect(int width, int height, int offsetX, int offsetY, Vec4f const& color, ResHandle matRes);
   WorldspaceRectHudElement* addWorldspaceRect(float width, float height, float offsetX, float offsetY, Vec4f const& color, ResHandle matRes);
   WorldspaceLineHudElement* addWorldspaceLine(int width, Vec4f const& color, ResHandle matRes);

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
