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
#include "radiant_stdutil.h"
#include "decal_node.h"
#include "stockpile_node.h"
#include "Horde3DRadiant.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;
using namespace ::Horde3D;

StockpileNode::StockpileNode(const StockpileTpl &terrainTpl) :
   SceneNode(terrainTpl),
   center_(0, nullptr)
{
   _renderable = true;
}

StockpileNode::~StockpileNode()
{
}

bool StockpileNode::InitExtension()
{
	Modules::sceneMan().registerType(SNT_StockpileNode, "Stockpile",
		                              StockpileNode::parsingFunc,
                                    StockpileNode::factoryFunc,
                                    StockpileNode::renderFunc);   
   return true;
}

SceneNodeTpl *StockpileNode::parsingFunc(std::map<std::string, std::string> &attribs)
{
   return new StockpileTpl("");
}

SceneNode *StockpileNode::factoryFunc(const SceneNodeTpl &nodeTpl)
{
   if (nodeTpl.type != SNT_StockpileNode) {
      return nullptr;
   }
   return new StockpileNode(static_cast<const StockpileTpl&>(nodeTpl));
}

void StockpileNode::renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                                 const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet)
{
}

void StockpileNode::UpdateShape(const csg::Cube3& bounds)
{
   if (!center_.first) {
      const DecalNode::Vertex cornerQuad[] = {
         DecalNode::Vertex(-0.5, 0, -0.5, 0, 0),
         DecalNode::Vertex(-0.5, 0,  0.5, 0, 1),
         DecalNode::Vertex( 0.5, 0,  0.5, 1, 1),
         DecalNode::Vertex( 0.5, 0, -0.5, 1, 0),
      };

      std::string name = getName();
      center_ = h3dRadiantCreateDecalNode(_handle, name + " center", "materials/stockpile/center.material.xml");
      for (int i = 0; i < 4; i++) {
         auto edge = h3dRadiantCreateDecalNode(_handle, name + " " + stdutil::ToString(i), "materials/stockpile/edge.material.xml");
         auto corner = h3dRadiantCreateDecalNode(_handle, name + " " + stdutil::ToString(i), "materials/stockpile/corner.material.xml");

         corner.second->UpdateShape(cornerQuad, 4);
         edges_.push_back(edge);
         corners_.push_back(corner);
      }
   }

   csg::Point3f p0, p1;
   for (int i = 0; i < 3; i++ ) {
      p0[i] = (float)bounds.GetMin()[i];
      p1[i] = (float)bounds.GetMax()[i];
   }
   
   //p0.y++;
   //p1.y++;
   // Move the corners...
   h3dSetNodeTransform(corners_[0].first, p0.x,     p0.y, p0.z,      0, 0,   0, 1, 1, 1);
   h3dSetNodeTransform(corners_[1].first, p0.x,     p0.y, p1.z - 1,  0, 90,  0, 1, 1, 1);
   h3dSetNodeTransform(corners_[3].first, p1.x - 1, p0.y, p1.z - 1,  0, 180, 0, 1, 1, 1);
   h3dSetNodeTransform(corners_[2].first, p1.x - 1, p0.y, p0.z,      0, 270, 0, 1, 1, 1);

   std::vector<DecalNode::Vertex> verts;
   float u, v;

   // z direction first...
   verts.clear();
   v = p1.z - p0.z - 2;
   verts.push_back(DecalNode::Vertex(-0.5, 0, -0.5,      0, 0));
   verts.push_back(DecalNode::Vertex( 0.5, 0, -0.5,      1, 0));
   verts.push_back(DecalNode::Vertex( 0.5, 0,  (float)(v - 0.5),  1, v));
   verts.push_back(DecalNode::Vertex(-0.5, 0,  (float)(v - 0.5),  0, v));
   edges_[0].second->UpdateShape(verts.data(), verts.size());
   edges_[1].second->UpdateShape(verts.data(), verts.size());
   h3dSetNodeTransform(edges_[0].first, p0.x,     p0.y, p0.z + 1,  0, 0,   0, 1, 1, 1);
   h3dSetNodeTransform(edges_[1].first, p1.x - 1, p0.y, p1.z - 2,  0, 180, 0, 1, 1, 1);
   //h3dSetNodeTransform(edges_[1].first, p1.x - 0.5,     p0.y, p1.z - 0.5,  0, 180, 0, 1, 1, 1);

   // now x...
   verts.clear();
   v = p1.x - p0.x - 2;
   verts.push_back(DecalNode::Vertex(-0.5, 0, -0.5,      0, 0));
   verts.push_back(DecalNode::Vertex( 0.5, 0, -0.5,      1, 0));
   verts.push_back(DecalNode::Vertex( 0.5, 0,  (float)(v - 0.5),  1, v));
   verts.push_back(DecalNode::Vertex(-0.5, 0,  (float)(v - 0.5),  0, v));
   edges_[2].second->UpdateShape(verts.data(), verts.size());
   edges_[3].second->UpdateShape(verts.data(), verts.size());
   h3dSetNodeTransform(edges_[2].first, p1.x - 2, p0.y, p0.z,      0, 270,  0, 1, 1, 1);
   h3dSetNodeTransform(edges_[3].first, p0.x + 1, p0.y, p1.z - 1,  0, 90,   0, 1, 1, 1);

   // Cre-create the center texture and edges...
   p0 += csg::Point3f(-0.5, 0.0, -0.5);
   p1 += csg::Point3f(-0.5, 0.0, -0.5);

   u = p1.x - p0.x;
   v = p1.z - p0.z;
   verts.clear();
   verts.push_back(DecalNode::Vertex(p0.x, p0.y, p0.z,  0, 0));
   verts.push_back(DecalNode::Vertex(p1.x, p0.y, p0.z,  u, 0));
   verts.push_back(DecalNode::Vertex(p1.x, p0.y, p1.z,  u, v));
   verts.push_back(DecalNode::Vertex(p0.x, p0.y, p1.z,  0, v));
   center_.second->UpdateShape(verts.data(), verts.size());


#if 0
   // Each corner gets p0 rotation of the image in the top left quadrant of the texture...
   auto addDecal = [](DecalNode* decal, float x, float y, float z, float dx, float dz, float tx0, float tz0, float tx1, float tz1) {
      std::vector<DecalNode::Vertex> verts;
      verts.push_back(DecalNode::Vertex(x,      y, z,      tx0, tz0));
      verts.push_back(DecalNode::Vertex(x + dx, y, z,      tx1, tz0));
      verts.push_back(DecalNode::Vertex(x + dx, y, z + dz, tx1, tz1));
      verts.push_back(DecalNode::Vertex(x,      y, z + dz, tx0, tz1));
      decal->UpdateShape(verts.data(), verts.size());
   };
   addDecal(corners_[0], p0.x,     p0.y, p0.z,       1, 1,     0, 0,  1, 1);
   addDecal(corners_[1], p0.x,     p0.y, p1.z - 1,   1, 1,     0, 1,  1, 0);
   addDecal(corners_[2], p1.x - 1, p0.y, p0.z,       1, 1,     1, 0,  0, 1);
   addDecal(corners_[3], p1.x - 1, p0.y, p1.z - 1,   1, 1,     1, 1,  0, 0);

   addDecal(edges_[0], p0.x + 1, p0.y, p0.z,       1, 1,     0, 0,  1, 1);
   addDecal(edges_[1], p0.x,     p0.y, p1.z - 2,   1, 1,     0, 1,  1, 0);
   addDecal(edges_[2], p1.x - 1, p0.y, p0.z + 1,   1, 1,     1, 0,  0, 1);
   addDecal(edges_[3], p1.x - 2, p0.y, p1.z - 1,   1, 1,     1, 1,  0, 0);

   // addDecal(edges_[0],   p0.x + 1, p0.y, p0.z,         u-1, 1,   0, 0,  1, u-1); Not even close..
   // addDecal(edges_[0],   p0.x + 1, p0.y, p0.z,         u-1, 1,   0, 0,  u-1, 1);  No...
#endif
}
