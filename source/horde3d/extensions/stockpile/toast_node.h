#ifndef _RADIANT_HORDE3D_TOAST_NODE_H
#define _RADIANT_HORDE3D_TOAST_NODE_H

#include "../../horde3d/Source/Horde3DEngine/egPrerequisites.h"
#include "../../horde3d/Source/Horde3DEngine/egScene.h"
#include "../extension.h"
#include "namespace.h"

BEGIN_RADIANT_HORDE3D_NAMESPACE

struct ToastTpl : public Horde3D::SceneNodeTpl
{
   ToastTpl(const std::string &name) :
      Horde3D::SceneNodeTpl(SNT_ToastNode, name)
   {
   }
};

class ToastNode : public Horde3D::SceneNode
{
public:

   static unsigned int vertexLayout;
   static Horde3D::MaterialResource* fontMaterial_;

	static Horde3D::SceneNodeTpl *parsingFunc(std::map< std::string, std::string > &attribs);
	static Horde3D::SceneNode *factoryFunc(const Horde3D::SceneNodeTpl &nodeTpl);
	static void renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
		                    const Horde3D::Frustum *frust1, const Horde3D::Frustum *frust2,
                          Horde3D::RenderingOrder::List order, int occSet);
   static bool ReadFntFile(const char* filename);
   static bool InitExtension();

public:
   struct Glyph {
      int x, y, width, height, xoffset, yoffset, xadvance;
   };

   struct Vertex {
      float             x, y, z;
      float             tu, tv;

      Vertex() { }
      Vertex(float _x, float _y, float _z, float _tu, float _tv) :
         x(_x), y(_y), z(_z), tu(_tu), tv(_tv) { }
   };

   void SetText(std::string text);

   void SetNode(H3DNode node) { node_ = node; }
   H3DNode GetNode() const { return node_; }

private:
	ToastNode(const ToastTpl &terrainTpl);
   ToastNode(H3DNode parent, std::string name);
	virtual ~ToastNode();

   void render();
   void onFinishedUpdate() override;

private:
   uint32                     indexBuffer_;
   uint32                     indexCount_;
   uint32                     vertexBuffer_;
   uint32                     vertexCount_;
   static Glyph               glyphs_[256];
   static int                 textureSize_;
   static float               textureWidth_;
   static float               textureHeight_;
   static int                 fontHeight_;
   H3DNode                    node_;
   Horde3D::BoundingBox       _bLocalBox;  // AABB in object space
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
