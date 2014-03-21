#ifndef _RADIANT_HORDE3D_DECAL_H
#define _RADIANT_HORDE3D_DECAL_H

#include "../../horde3d/Source/Horde3DEngine/egPrerequisites.h"
#include "../../horde3d/Source/Horde3DEngine/egScene.h"
#include "../extension.h"
#include "namespace.h"

BEGIN_RADIANT_HORDE3D_NAMESPACE

struct DecalTpl : public Horde3D::SceneNodeTpl
{
   DecalTpl(const std::string &name) :
      Horde3D::SceneNodeTpl(SNT_DecalNode, name)
   {
   }
};

class DecalNode : public Horde3D::SceneNode
{
public:
	~DecalNode();

   static unsigned int vertexLayout;
   static Horde3D::MaterialResource* material;

	static Horde3D::SceneNodeTpl *parsingFunc(std::map< std::string, std::string > &attribs);
	static Horde3D::SceneNode *factoryFunc(const Horde3D::SceneNodeTpl &nodeTpl);
	static void renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
		                    const Horde3D::Frustum *frust1, const Horde3D::Frustum *frust2,
                          Horde3D::RenderingOrder::List order, int occSet, int lodLevel);

   static bool InitExtension();

public:
   struct Vertex {
      float             x, y, z;
      float             tu, tv;

      Vertex(float _x, float _y, float _z, float _tu, float _tv) :
         x(_x), y(_y), z(_z), tu(_tu), tv(_tv) { }
   };

   Horde3D::MaterialResource* GetMaterial() { return material_; }
   bool SetMaterial(std::string name);
   void UpdateShape(const Vertex* verts, int cVerts);

private:
	DecalNode(const DecalTpl &terrainTpl);
   void render();
   void onFinishedUpdate() override;

private:
   static uint32              indexBuffer_;
   static uint32              indexCount_;
   uint32                     vertexBuffer_;
   uint32                     vertexCount_;
   Horde3D::BoundingBox       _bLocalBox;  // AABB in object space
   Horde3D::MaterialResource* material_;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
