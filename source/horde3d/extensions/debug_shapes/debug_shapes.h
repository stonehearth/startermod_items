#ifndef _RADIANT_HORDE3D_EXTENSIONS_DEBUG_SHAPES_H
#define _RADIANT_HORDE3D_EXTENSIONS_DEBUG_SHAPES_H

#include "egPrerequisites.h"
#include "egScene.h"
#include "csg/region.h"
#include "csg/color.h"
#include "../namespace.h"
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
   static MaterialResource* default_material;

	static SceneNodeTpl *parsingFunc(std::map< std::string, std::string > &attribs);
	static SceneNode *factoryFunc(const SceneNodeTpl &nodeTpl);
	static void renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
		                    const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet);
   void setParamI( int param, int value );

public:
   void decode(const protocol::shapelist &shapes);
   void clear();
   void createBuffers();
   void add_aabb(const csg::Cube3f& aabb, const csg::Color4& color);
   void add_region(const csg::Region3& rgn, const csg::Color4& color);
	void add_quad_xz(const csg::Point3f &p0, const csg::Point3f &p1, const csg::Color4 &color);
   void add_line(const csg::Point3f &p0, const csg::Point3f &p1, const csg::Color4 &c);
   void add_triangle(const csg::Point3f points[3], const csg::Color4 &c);
   void add_quad(const csg::Point3f points[4], const csg::Color4 &c);
   void SetMaterial(MaterialResource *material);
   MaterialResource* GetMaterial() { return material_; }

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

      Vertex(const csg::Point3f& v, const csg::Color4& c) {
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
   void decode_box(const protocol::box &box);
   void decode_region(const protocol::region3i &region);
   void decode_coord(const protocol::coord &coord);
   void onFinishedUpdate() override;
   
private:
   uint32                  vertexBuffer_;
   uint32                  vertexBufferSize_;
   std::vector<Primitives> primitives_;
   std::vector<Vertex>     triangles_;
   std::vector<Vertex>     lines_;
   MaterialResource*       material_;
   Horde3D::BoundingBox    _bLocalBox;  // AABB in object space  
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
