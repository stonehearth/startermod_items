#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <fstream>
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "egMaterial.h"
#include "egCamera.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"

#if defined(ASSERT)
#  undef ASSERT
#endif

#include "radiant.h"
#include "toast_node.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;
using namespace ::Horde3D;

unsigned int ToastNode::vertexLayout;
MaterialResource* ToastNode::fontMaterial_;
ToastNode::Glyph ToastNode::glyphs_[256];
int ToastNode::textureSize_;
int ToastNode::fontHeight_;
float ToastNode::textureWidth_;
float ToastNode::textureHeight_;
static const float fontScale = 1.0f / 32.0f;

ToastNode::ToastNode(const ToastTpl &terrainTpl) :
   SceneNode(terrainTpl),
   vertexBuffer_(0),
   indexBuffer_(0),
   vertexCount_(0),
   indexCount_(0)
{
   _renderable = true;
}

ToastNode::~ToastNode()
{
}

bool ToastNode::InitExtension()
{
	Modules::sceneMan().registerType(SNT_ToastNode, "Toast",
		                              ToastNode::parsingFunc,
                                    ToastNode::factoryFunc,
                                    ToastNode::renderFunc);

	VertexLayoutAttrib attribs[2] = {
		"vertPos",     0, 3, 0,
		"texCoords0",  0, 2, 12,
	};
	ToastNode::vertexLayout = gRDI->registerVertexLayout(2, attribs);

   H3DRes mat = h3dAddResource(H3DResTypes::Material, "fonts/toast_text.material.xml", 0);
	Resource *matRes =  Modules::resMan().resolveResHandle(mat);
	if (matRes == 0x0 || matRes->getType() != ResourceTypes::Material ) {
      return 0;
   }

   H3DRes texture = h3dGetResParamI(mat, MaterialResData::SamplerElem, 0, MaterialResData::SampTexResI);
   if (texture) {
      textureWidth_  = (float)h3dGetResParamI(texture, TextureResData::ImageElem, 0, TextureResData::ImgWidthI);
      textureHeight_ = (float)h3dGetResParamI(texture, TextureResData::ImageElem, 0, TextureResData::ImgHeightI);
   }
   fontMaterial_ = (MaterialResource *)matRes;


   // create the font sheet...
   if (!ReadFntFile2("horde\\fonts\\fontdinerdotcom_huggable_regular_20.xml")) {
      return false;
   }

   return true;
}

bool ToastNode::ReadFntFile(const char* filename)
{
   std::ifstream in(filename);
   if (!in.good()) {
      return false;
   }


   memset(glyphs_, 0, sizeof glyphs_);

   while (in.good()) {
      char line[1024];
      in.getline(line, sizeof line);
      if (strncmp(line, "char ", 5) == 0) { 
         int id, x, y, w, h, xo, yo, xa, pg, chnl;
         if (sscanf(line, "char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d page=%d chnl=%d",
                    &id, &x, &y, &w, &h, &xo, &yo, &xa, &pg, &chnl) != 10) {
               return false;
         }
         Glyph& glyph = glyphs_[id];
         glyph.x = x;
         glyph.y = y;
         glyph.width = w;
         glyph.height = h;
         glyph.xadvance = xa;
         glyph.xoffset = xo;
         glyph.yoffset = yo;

         // we fudge the xadvance by the size of the black border we addeded
         // around each character after exporting from FontBuilder
         glyph.xadvance += 4;
      }
   }
   return true;
}

bool ToastNode::ReadFntFile2(const char* filename)
{
   std::ifstream in(filename);
   if (!in.good()) {
      return false;
   }

#define DISCARD "\"%*[^\"]\""
#define KEEP "\"%[^\"]\""

   memset(glyphs_, 0, sizeof glyphs_);
   while (in.good()) {
      char line[1024];
      in.getline(line, sizeof line);
      if (strncmp(line, "<Font", 5) == 0) {
         char height_str[64];
         if (sscanf(line, "<Font size=" DISCARD " family=" DISCARD " height=" KEEP, height_str) == 1) {
            fontHeight_ = atoi(height_str);
         }
      } else if (strncmp(line, " <Char", 6) == 0) { 
         char code[32];
         int id = 0, x, y, w, h, xo, yo, xa;
         if (sscanf(line, " <Char width=\"%d\" offset=\"%d %d\" rect=\"%d %d %d %d\" code=" KEEP,
                    &xa, &xo, &yo, &x, &y, &w, &h, code) != 8) {
            return false;
         }
         if (*code == '&') {
            if (strcmp(code, "&quot;") == 0) {
               id = '\'';
            } else if (strcmp(code, "&amp;") == 0) {
               id = '&';
            } else if (strcmp(code, "&lt;") == 0) {
               id = '<';
            } else if (strcmp(code, "&gt;") == 0) {
               id = '>';
            }
         } else {
            id = *code;
         }
         if (id) {
            Glyph& glyph = glyphs_[id];
            glyph.x = x;
            glyph.y = y;
            glyph.width = w;
            glyph.height = h;
            glyph.xadvance = xa;
            glyph.xoffset = xo;
            glyph.yoffset = yo;
         }
      }
   }
   return true;

#undef DISCARD
#undef KEEP
}

SceneNodeTpl *ToastNode::parsingFunc(std::map<std::string, std::string> &attribs)
{
   return new ToastTpl("");
}

SceneNode *ToastNode::factoryFunc(const SceneNodeTpl &nodeTpl)
{
   if (nodeTpl.type != SNT_ToastNode) {
      return nullptr;
   }
   return new ToastNode(static_cast<const ToastTpl&>(nodeTpl));
}

void ToastNode::renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                           const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet)
{
   bool offsetSet = false;
   for (const auto &entry : Modules::sceneMan().getRenderableQueue()) {
      if (entry.type != SNT_ToastNode) {
         continue;
      }
      ToastNode *toast = (ToastNode *)entry.node;

      if (!fontMaterial_ || !fontMaterial_->isOfClass(theClass)) {
         continue;
      }
		if (!Modules::renderer().setMaterial(fontMaterial_, shaderContext)) {
         continue;
      }
      toast->render();
   }
}

void ToastNode::render()
{
   if (!vertexCount_) {
      return;
   }

   // Bind geometry and apply vertex layout
   // Set uniforms
   ShaderCombination *curShader = Modules::renderer().getCurShader();
   if (curShader->uni_worldMat >= 0) {
      gRDI->setShaderConst(curShader->uni_worldMat, CONST_FLOAT44, &_absTrans.x[0]);
   }

   if (curShader->uni_nodeId >= 0) {
      float id = (float)getHandle();
      gRDI->setShaderConst(curShader->uni_nodeId, CONST_FLOAT, &id);
   }

   gRDI->setIndexBuffer(indexBuffer_, IDXFMT_16);
   gRDI->setVertexBuffer(0, vertexBuffer_, 0, sizeof(Vertex));
   gRDI->setVertexLayout(vertexLayout);
   gRDI->drawIndexed(PRIM_TRILIST, 0, indexCount_, 0, vertexCount_);

   Modules::stats().incStat(EngineStats::BatchCount, 1);

   gRDI->setVertexLayout(0);
}



void ToastNode::SetText(std::string text)
{
   int total_width = 0;
   for (char c: text) {
      total_width += glyphs_[c].xadvance;
   }
   float start = -((float)total_width) / 2;

   _bLocalBox.min.x = start;
   _bLocalBox.max.x = start + total_width;
   _bLocalBox.min.y = 0;
   _bLocalBox.max.y = 0;
   _bLocalBox.min.z = 0;
   _bLocalBox.max.z = 0;

   vertexCount_ = text.size() * 4;
   indexCount_ = text.size() * 6;
   std::vector<Vertex> verts(vertexCount_);
   std::vector<unsigned short> indices(indexCount_);

   auto tcoord = [&](int x, float size) -> float {
      return (((float) x) + 0.5f) / size;
   };

   int vi = 0, ii = 0;
   for (unsigned int i = 0; i < text.size(); i++) {
      const Glyph& glyph = glyphs_[text[i]];
      float x0 = start + glyph.xoffset;
      float x1 = x0 + glyph.width;
      float y0 = fontHeight_ - (float)glyph.yoffset;
      float y1 = y0 - (float)glyph.height;
      float u0 = tcoord(glyph.x, textureWidth_);
      float u1 = tcoord(glyph.x + glyph.width, textureWidth_);
      float v0 = tcoord(glyph.y, textureHeight_);
      float v1 = tcoord(glyph.y + glyph.height, textureHeight_);

      verts[vi++] = Vertex(fontScale * x0, fontScale * y0, 0, u0, v0);
      verts[vi++] = Vertex(fontScale * x0, fontScale * y1, 0, u0, v1);
      verts[vi++] = Vertex(fontScale * x1, fontScale * y1, 0, u1, v1);
      verts[vi++] = Vertex(fontScale * x1, fontScale * y0, 0, u1, v0);

      int voffset = (i * 4);
      indices[ii++] = voffset + 0;  // first triangle...
      indices[ii++] = voffset + 1;
      indices[ii++] = voffset + 2;
      indices[ii++] = voffset + 2;  // second triangle...
      indices[ii++] = voffset + 3;
      indices[ii++] = voffset + 0;

      _bLocalBox.max.y = std::max(_bLocalBox.max.y, (float)glyph.height);
      start += glyph.xadvance;
   }
	gRDI->destroyBuffer(vertexBuffer_);
	gRDI->destroyBuffer(indexBuffer_);
   vertexBuffer_ = 0;
   indexBuffer_ = 0;

   if (vertexCount_) {
      indexBuffer_ = gRDI->createIndexBuffer(indexCount_ * sizeof(unsigned short), indices.data());
      vertexBuffer_ = gRDI->createVertexBuffer(vertexCount_ * sizeof(Vertex), verts.data());
   }
}

void ToastNode::onFinishedUpdate()
{
   _bBox = _bLocalBox;

   Vec3f trans = _absTrans.getTrans();
   Vec3f scale = _absTrans.getScale();
   _absTrans = Matrix4f::TransMat(trans.x, trans.y, trans.z) * Matrix4f::ScaleMat(scale.x, scale.y, scale.z);
   _bBox.transform(_absTrans);
}
