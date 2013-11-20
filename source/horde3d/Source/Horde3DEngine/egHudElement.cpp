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

#include "egHudElement.h"
#include "egCamera.h"
#include "egMaterial.h"
#include "egModules.h"
#include "egRenderer.h"
#include "egCom.h"
#include <cstring>

#include "utDebug.h"




namespace Horde3D {

using namespace std;


const BoundingBox& HudElement::getBounds() const
{
   return bounds_;
}

void HudElement::draw(const std::string &shaderContext, const std::string &theClass, Matrix4f& worldMat)
{

}



struct ScreenspaceRectVertex
{
   Vec4f pos;
   float texU, texV;
   Vec4f color;
};

struct WorldspaceRectVertex
{
   Vec3f pos;
   float texU, texV;
   Vec4f color;
};

WorldspaceLineElement::WorldspaceLineElement(Vec3f startPoint, Vec3f endPoint, int width, Vec4f color, ResHandle matRes)
{

}



void WorldspaceLineElement::updateGeometry(const Matrix4f& absTrans)
{
   // Soon....
}





ScreenspaceRectHudElement::ScreenspaceRectHudElement(int width, int height, int xOffset, int yOffset, Vec4f color, ResHandle matRes)
{
   // Create cube geometry array
   rectVBO_ = gRDI->createVertexBuffer( sizeof(ScreenspaceRectVertex) * 4, 0x0 );

   // Create cube geometry indices.
   uint16 cubeInds[6] = {
      0, 1, 2,
      0, 2, 3
   };
   rectIdxBuf_ = gRDI->createIndexBuffer(6 * sizeof(uint16), cubeInds);

   width_ = (float)width;
   height_ = (float)height;
   offsetX_ = (float)xOffset;
   offsetY_ = (float)yOffset;
   color_ = color;

   MaterialResource* mr = (MaterialResource*)Modules::resMan().resolveResHandle( matRes );
   materialRes_ = mr;
}


void ScreenspaceRectHudElement::draw(const std::string &shaderContext, const std::string &theClass, Matrix4f& worldMat)
{
   if (!materialRes_->isOfClass(theClass)) 
   {
      return;
   }

   if (!Modules::renderer().setMaterial(materialRes_, shaderContext))
   {
      return;
   }
   gRDI->setVertexLayout(Modules::renderer().getClipspaceLayout());

   gRDI->setVertexBuffer(0, rectVBO_, 0, sizeof(ScreenspaceRectVertex));
   gRDI->setIndexBuffer(rectIdxBuf_, IDXFMT_16);

   gRDI->drawIndexed(PRIM_TRILIST, 0, 6, 0, 4);
}


void ScreenspaceRectHudElement::updateGeometry(const Matrix4f& absTrans)
{
   bounds_ = BoundingBox();
   int vpWidth, vpHeight, _;

   gRDI->getViewport(&_, &_, &vpWidth, &vpHeight);

   const Matrix4f& viewMat = Modules::renderer().getCurCamera()->getViewMat();
   const Matrix4f& projMat = Modules::renderer().getCurCamera()->getProjMat();
   const Matrix4f clipMat = projMat * viewMat * absTrans;
   const Matrix4f invClipMat = (projMat * viewMat).inverted();
   const Vec4f origin = clipMat * Vec4f(0, 0, 0, 1);

   Vec4f screenPos;
   ScreenspaceRectVertex* verts = (ScreenspaceRectVertex*)gRDI->mapBuffer(rectVBO_);
   float xScale = origin.w * 2.0f / vpWidth;
   float yScale = origin.w * 2.0f / vpHeight;

   screenPos = Vec4f(offsetX_, offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   verts[0].pos = origin + screenPos;
   verts[0].texU = 0; verts[0].texV = 1;
   verts[0].color = color_;
   bounds_.addPoint((invClipMat * verts[0].pos).xyz());

   screenPos = Vec4f(offsetX_, height_ + offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   verts[1].pos = origin + screenPos;
   verts[1].texU = 0; verts[1].texV = 0;
   verts[1].color = color_;
   bounds_.addPoint((invClipMat * verts[1].pos).xyz());

   screenPos = Vec4f(width_ + offsetX_, height_ + offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   verts[2].pos = origin + screenPos;
   verts[2].texU = 1; verts[2].texV = 0;
   verts[2].color = color_;
   bounds_.addPoint((invClipMat * verts[2].pos).xyz());

   screenPos = Vec4f(width_ + offsetX_, offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   verts[3].pos = origin + screenPos;
   verts[3].texU = 1; verts[3].texV = 1;
   verts[3].color = color_;
   bounds_.addPoint((invClipMat * verts[3].pos).xyz());

   gRDI->unmapBuffer(rectVBO_);
}


ScreenspaceRectHudElement::~ScreenspaceRectHudElement()
{
   gRDI->destroyBuffer(rectVBO_);
   gRDI->destroyBuffer(rectIdxBuf_);
   materialRes_ = 0x0;
}



WorldspaceRectHudElement::WorldspaceRectHudElement(int width, int height, int xOffset, int yOffset, Vec4f color, ResHandle matRes)
{
   // Create cube geometry array
   rectVBO_ = gRDI->createVertexBuffer( sizeof(WorldspaceRectVertex) * 4, 0x0 );

   // Create cube geometry indices.
   uint16 cubeInds[6] = {
      0, 1, 2,
      0, 2, 3
   };
   rectIdxBuf_ = gRDI->createIndexBuffer(6 * sizeof(uint16), cubeInds);

   width_ = (float)width;
   height_ = (float)height;
   offsetX_ = (float)xOffset;
   offsetY_ = (float)yOffset;
   color_ = color;

   MaterialResource* mr = (MaterialResource*)Modules::resMan().resolveResHandle( matRes );
   materialRes_ = mr;

   WorldspaceRectVertex* verts = (WorldspaceRectVertex*)gRDI->mapBuffer(rectVBO_);

   verts[0].pos = Vec3f(-width_, -height_, 0);
   verts[0].texU = 0; verts[0].texV = 1;
   verts[0].color = color_;

   verts[1].pos = Vec3f(-width_, height_, 0);
   verts[1].texU = 0; verts[1].texV = 0;
   verts[1].color = color_;

   verts[2].pos = Vec3f(width_, height_, 0);
   verts[2].texU = 1; verts[2].texV = 0;
   verts[2].color = color_;

   verts[3].pos = Vec3f(width_, -height_, 0);
   verts[3].texU = 1; verts[3].texV = 1;
   verts[3].color = color_;

   gRDI->unmapBuffer(rectVBO_);
}


void WorldspaceRectHudElement::draw(const std::string &shaderContext, const std::string &theClass, Matrix4f& worldMat)
{
   if (!materialRes_->isOfClass(theClass)) 
   {
      return;
   }

   if (!Modules::renderer().setMaterial(materialRes_, shaderContext))
   {
      return;
   }
 
   // World transformation
   if( Modules::renderer().getCurShader()->uni_worldMat >= 0 )
   {
      gRDI->setShaderConst( Modules::renderer().getCurShader()->uni_worldMat, CONST_FLOAT44, &worldMat.x[0] );
   }
   gRDI->setVertexLayout(Modules::renderer().getPosColTexLayout());

   gRDI->setVertexBuffer(0, rectVBO_, 0, sizeof(WorldspaceRectVertex));
   gRDI->setIndexBuffer(rectIdxBuf_, IDXFMT_16);

   gRDI->drawIndexed(PRIM_TRILIST, 0, 6, 0, 4);
}


void WorldspaceRectHudElement::updateGeometry(const Matrix4f& absTrans)
{
   bounds_ = BoundingBox();
   const Matrix4f& viewMat = Modules::renderer().getCurCamera()->getViewMat();
   const Matrix4f& invView = viewMat.inverted();
   const Vec3f origin = absTrans.getTrans();

   Vec3f p = Vec3f(-width_, -height_, 0);
   bounds_.addPoint(origin + (invView.mult33Vec(p)));

   p = Vec3f(-width_, height_, 0);
   bounds_.addPoint(origin + (invView.mult33Vec(p)));

   p = Vec3f(width_, height_, 0);
   bounds_.addPoint(origin + (invView.mult33Vec(p)));

   p = Vec3f(width_, -height_, 0);
   bounds_.addPoint(origin + (invView.mult33Vec(p)));
}


WorldspaceRectHudElement::~WorldspaceRectHudElement()
{
   gRDI->destroyBuffer(rectVBO_);
   gRDI->destroyBuffer(rectIdxBuf_);
   materialRes_ = 0x0;
}




HudElementNode::HudElementNode( const HudElementNodeTpl &hudElementTpl ) :
	SceneNode( hudElementTpl )
{
   _renderable = true;
}


HudElementNode::~HudElementNode()
{
   for (auto& e : elements_)
   {
      delete e;
   }
   elements_.clear();
}


SceneNodeTpl *HudElementNode::parsingFunc( map< string, std::string > &attribs )
{
   bool result = true;

   HudElementNodeTpl *hudTpl = new HudElementNodeTpl( "");

   return hudTpl;
}


SceneNode *HudElementNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
   if( nodeTpl.type != SceneNodeTypes::HudElement ) return 0x0;

   return new HudElementNode( *(HudElementNodeTpl *)&nodeTpl );
}


int HudElementNode::getParamI( int param )
{
   return SceneNode::getParamI( param );
}


void HudElementNode::setParamI( int param, int value )
{
   SceneNode::setParamI( param, value );
}


float HudElementNode::getParamF( int param, int compIdx )
{
   return SceneNode::getParamF( param, compIdx );
}


void HudElementNode::setParamF( int param, int compIdx, float value )
{
   SceneNode::setParamF( param, compIdx, value );
}


void HudElementNode::onPostUpdate()
{
   BoundingBox newBounds;
   for (const auto& element : elements_)
   {
      element->updateGeometry(_absTrans);
      newBounds.makeUnion(element->getBounds());
   }

   _bBox = newBounds;
}


void HudElementNode::onFinishedUpdate()
{
   // Hud elements are view-dependent, so we need them to constantly be updated
   // whenever the view changes.  For now, just always mark them as dirty.
   markDirty();
}


ScreenspaceRectHudElement* HudElementNode::addScreenspaceRect(int width, int height, int offsetX, int offsetY, Vec4f color, ResHandle matRes)
{
   ScreenspaceRectHudElement* result = new ScreenspaceRectHudElement(width, height, offsetX, offsetY, color, matRes);
   elements_.push_back(result);
   return result;
}

WorldspaceRectHudElement* HudElementNode::addWorldspaceRect(int width, int height, int offsetX, int offsetY, Vec4f color, ResHandle matRes)
{
   WorldspaceRectHudElement* result = new WorldspaceRectHudElement(width, height, offsetX, offsetY, color, matRes);
   elements_.push_back(result);
   return result;
}

const std::vector<HudElement*>& HudElementNode::getSubElements() const {
   return elements_;
}


}  // namespace
