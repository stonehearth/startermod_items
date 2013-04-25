#ifndef _RADIANT_HORDE3D_TERRAIN_NODE_H
#define _RADIANT_HORDE3D_TERRAIN_NODE_H

#include "../../horde3d/Source/Horde3DEngine/egPrerequisites.h"
#include "../../horde3d/Source/Horde3DEngine/egScene.h"
#include "../extension.h"
#include "math3d.h"
#include "namespace.h"
#include "csg/meshtools.h"

BEGIN_RADIANT_HORDE3D_NAMESPACE

class TerrainNode : public Horde3D::SceneNode
{
public:
   static unsigned int vertexLayout;
   static Horde3D::MaterialResource* material_;
   static std::unordered_map<int, Horde3D::MaterialResource*> material_map_;

	static void renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
		                    const Horde3D::Frustum *frust1, const Horde3D::Frustum *frust2,
                          Horde3D::RenderingOrder::List order, int occSet);
   static bool InitExtension();

public:
	TerrainNode(int type, csg::mesh_tools::mesh const& m);

   void SetNode(H3DNode node) { node_ = node; }
   H3DNode GetNode() const { return node_; }
   bool checkIntersection( const Horde3D::Vec3f &rayOrig, const Horde3D::Vec3f &rayDir, Horde3D::Vec3f &intsPos, Horde3D::Vec3f &intsNorm ) const override;

private:
	virtual ~TerrainNode();

   void CreateRenderMesh(csg::mesh_tools::mesh const& m);
   void render();
   void onFinishedUpdate() override;

private:
   csg::mesh_tools::mesh      mesh_;
   uint32                     indexBuffer_;
   uint32                     indexCount_;
   uint32                     vertexBuffer_;
   uint32                     vertexCount_;
   H3DNode                    node_;
   int                        terrain_type_;
   Horde3D::BoundingBox       _bLocalBox;  // AABB in object space
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
