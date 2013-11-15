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



RectHudElement::RectHudElement(int width, int height, int xOffset, int yOffset, ResHandle matRes)
{
   const int numRectAttributes = 1;
   VertexLayoutAttrib attribsRect[numRectAttributes] = {
      {"vertPos", 0, 4, 0}
   };
   vlRect_ = gRDI->registerVertexLayout( numRectAttributes, attribsRect);

   // Create cube geometry array
   rectVBO_ = gRDI->createVertexBuffer( sizeof(float) * 4 * 4, 0x0 );

   // Create cube geometry indices.
   uint16 cubeInds[6] = {
      0, 1, 2,
      0, 2, 3
   };
   rectIdxBuf_ = gRDI->createIndexBuffer(6 * sizeof(uint16), cubeInds);

   width_ = width;
   height_ = height;
   offsetX_ = xOffset;
   offsetY_ = yOffset;

   MaterialResource* mr = (MaterialResource*)Modules::resMan().resolveResHandle( matRes );
   materialRes_ = mr;
}


void RectHudElement::draw(const std::string &shaderContext, const std::string &theClass)
{
   if (!materialRes_->isOfClass(theClass)) 
   {
      return;
   }

   if (!Modules::renderer().setMaterial(materialRes_, shaderContext))
   {
      return;
   }
   gRDI->setVertexLayout(vlRect_);

   gRDI->setVertexBuffer(0, rectVBO_, 0, sizeof(float) * 4);
   gRDI->setIndexBuffer(rectIdxBuf_, IDXFMT_16);

   // Shader uniforms
   //ShaderCombination *curShader = Modules::renderer().getCurShader();

   gRDI->drawIndexed(PRIM_TRILIST, 0, 6, 0, 4);
}


void RectHudElement::updateGeometry(const Matrix4f& absTrans)
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
   Vec4f* verts = (Vec4f*)gRDI->mapBuffer(rectVBO_);
   float xScale = origin.w * 2.0f / vpWidth;
   float yScale = origin.w * 2.0f / vpHeight;

   screenPos = Vec4f(offsetX_, offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   Vec4f worldSpace = invClipMat * (origin + screenPos);
   verts[0] = origin + screenPos;
   bounds_.addPoint(Vec3f(worldSpace.x, worldSpace.y, worldSpace.z));

   screenPos = Vec4f(offsetX_, height_ + offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   worldSpace = invClipMat * (origin + screenPos);
   verts[1] = origin + screenPos;
   bounds_.addPoint(Vec3f(worldSpace.x, worldSpace.y, worldSpace.z));

   screenPos = Vec4f(width_ + offsetX_, height_ + offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   worldSpace = invClipMat * (origin + screenPos);
   verts[2] = origin + screenPos;
   bounds_.addPoint(Vec3f(worldSpace.x, worldSpace.y, worldSpace.z));

   screenPos = Vec4f(width_ + offsetX_, offsetY_, 0, 0);
   screenPos.x *= xScale;
   screenPos.y *= yScale;
   worldSpace = invClipMat * (origin + screenPos);
   verts[3] = origin + screenPos;
   bounds_.addPoint(Vec3f(worldSpace.x, worldSpace.y, worldSpace.z));

   gRDI->unmapBuffer(rectVBO_);
}


RectHudElement::~RectHudElement()
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


RectHudElement* HudElementNode::addRect(int width, int height, int offsetX, int offsetY, ResHandle matRes)
{
   RectHudElement* result = new RectHudElement(width, height, offsetX, offsetY, matRes);
   elements_.push_back(result);
   return result;
}


const std::vector<HudElement*>& HudElementNode::getSubElements() const {
   return elements_;
}


}  // namespace
