#ifndef _RADIANT_HORDE3D_EXTENSIONS_DEBUG_SHAPES_H
#define _RADIANT_HORDE3D_EXTENSIONS_DEBUG_SHAPES_H

#include "egPrerequisites.h"
#include "egScene.h"
#include "math3d.h"
#include "csg/region.h"
#include "namespace.h"
#include "radiant.pb.h"
#include "../extension.h"

using namespace ::Horde3D;

BEGIN_RADIANT_HORDE3D_NAMESPACE

struct DebugShapesTpl : public SceneNodeTpl
{
   DebugShapesTpl(const std::string &name) :
      SceneNodeTpl(SNT_DebugShapesNode, name)
   {
   }
};


class DebugShapesNode : public SceneNode
{
public:
	~DebugShapesNode();

   static uint32 vertexLayout;
   static MaterialResource* material;

	static SceneNodeTpl *parsingFunc(std::map< std::string, std::string > &attribs);
	static SceneNode *factoryFunc(const SceneNodeTpl &nodeTpl);
	static void renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
		                    const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet);


public:
   void decode(const protocol::shapelist &shapes);
   void clear();
   void createBuffers();
   void add_aabb(const math3d::aabb& aabb, const math3d::color4& color);
   void add_region(const csg::Region3& rgn, const math3d::color4& color);
	void add_quad_xz(const math3d::point3 &p0, const math3d::point3 &p1, const math3d::color4 &color);
   void add_line(const math3d::point3 &p0, const math3d::point3 &p1, const math3d::color4 &c);
   void add_triangle(const math3d::point3 points[3], const math3d::color4 &c);
   void add_quad(const math3d::point3 points[4], const math3d::color4 &c);

private:
   struct Primitives {
      RDIPrimType       type;
      uint32            vertexStart;
      uint32            vertexCount;
      Primitives(RDIPrimType t, uint32 s, uint32 c) :
         type(t), vertexStart(s), vertexCount(c) { }
   };

   struct Vertex {
      float             position[3];
      float             color[4];

      Vertex(const math3d::point3& v, const math3d::color4& c) {
         for (int i = 0; i < 3; i++) {
            position[i] = v[i];
         }
         color[0] = c.r / 255.0f;
         color[1] = c.g / 255.0f;
         color[2] = c.b / 255.0f;
         color[3] = c.a / 255.0f;
      }
   };

	DebugShapesNode(const DebugShapesTpl &terrainTpl);
   void render();
   bool empty();
   void decode_line(const protocol::line &line);
   void decode_quad(const protocol::quad &quad);
   void decode_coord(const protocol::coord &coord);
   void decode_box(const protocol::box &box);
   void decode_region(const protocol::region &region);
   void onFinishedUpdate() override;
   
private:
   uint32                  vertexBuffer_;
   uint32                  vertexBufferSize_;
   std::vector<Primitives> primitives_;
   std::vector<Vertex>     triangles_;
   std::vector<Vertex>     lines_;
   Horde3D::BoundingBox    _bLocalBox;  // AABB in object space  
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
