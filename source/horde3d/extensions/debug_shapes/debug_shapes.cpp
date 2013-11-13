#include "Horde3D.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "egMaterial.h"
#include "egCamera.h"

#if defined(ASSERT)
#  undef ASSERT
#endif

#include "radiant.h"
#include "debug_shapes.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;

radiant::uint32 DebugShapesNode::vertexLayout;
MaterialResource* DebugShapesNode::default_material;

DebugShapesNode::DebugShapesNode(const DebugShapesTpl &terrainTpl) :
   SceneNode(terrainTpl),
   vertexBuffer_(0),
   vertexBufferSize_(0)
{
   SetMaterial(default_material);
   _renderable = true;
}

DebugShapesNode::~DebugShapesNode()
{
}

SceneNodeTpl *DebugShapesNode::parsingFunc(std::map<std::string, std::string> &attribs)
{
   return new DebugShapesTpl("");
}

SceneNode *DebugShapesNode::factoryFunc(const SceneNodeTpl &nodeTpl)
{
   if (nodeTpl.type != SNT_DebugShapesNode) {
      return nullptr;
   }
   return new DebugShapesNode(static_cast<const DebugShapesTpl&>(nodeTpl));
}

void DebugShapesNode::renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                                 const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet)
{
   bool first = true;
   float old_line_width = 1.0f;

   // Loop through debug shape queue
   for (const auto &entry : Modules::sceneMan().getRenderableQueue()) {
      if (entry.type != SNT_DebugShapesNode) {
         continue;
      }
      DebugShapesNode *debugShapes = (DebugShapesNode *)entry.node;
      if (debugShapes->empty()) {
         continue;
      }

      //Modules::renderer().setShaderComb(&debugViewShader);
      //Modules::renderer().commitGeneralUniforms();
      if (first) {
         MaterialResource* material = debugShapes->GetMaterial();
         if (!material->isOfClass(theClass)) {
            continue;
         }
		   if (!Modules::renderer().setMaterial(material, shaderContext)) {
            continue;
         }
         first = false;
         glGetFloatv(GL_LINE_WIDTH, &old_line_width);
      }
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(-1, -1);
      glLineWidth(2.0f);
      debugShapes->render();
   }
   if (!first) {
      glDisable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0, 0);
      glLineWidth(old_line_width);
   }
}

void DebugShapesNode::setParamI( int param, int value )
{
   if (param == H3DMesh::MatResI) {
		Resource* res = Modules::resMan().resolveResHandle( value );
		if (res && res->getType() == ResourceTypes::Material ) {
         SetMaterial((MaterialResource *)res);
		}
   }
}

void DebugShapesNode::SetMaterial(MaterialResource *material)
{
   material_ = material;
   _sortKey = (float)material->getHandle();
}

void DebugShapesNode::render()
{
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

   gRDI->setIndexBuffer(0, IDXFMT_16);
   gRDI->setVertexBuffer(0, vertexBuffer_, 0, sizeof(Vertex));
   gRDI->setVertexLayout(vertexLayout);

   for (const auto& entry : primitives_) {
      gRDI->draw(entry.type,
                 entry.vertexStart, entry.vertexCount);
   }
   Modules::stats().incStat(EngineStats::BatchCount, (float)primitives_.size());

   gRDI->setVertexLayout(0);
}

void DebugShapesNode::clear()
{
   primitives_.clear();
   triangles_.clear();
   lines_.clear();
}

void DebugShapesNode::createBuffers()
{
	gRDI->destroyBuffer(vertexBuffer_);
   vertexBuffer_ = 0;
   vertexBufferSize_ = 0;

   // Accumulate evertying into a single buffer
   int start = 0, last = 0;

   std::vector<Vertex> verts;
   if (!lines_.empty()) {
      start = last;
      last = start + lines_.size();
      std::copy(lines_.begin(), lines_.end(), std::back_inserter(verts));
      primitives_.push_back(Primitives(PRIM_LINES, start, last));
   }
   if (!triangles_.empty()) {
      start = last;
      last = start + triangles_.size();
      std::copy(triangles_.begin(), triangles_.end(), std::back_inserter(verts));
      primitives_.push_back(Primitives(PRIM_TRILIST, start, last));
   }

   if (!primitives_.empty()) {
      vertexBufferSize_ = verts.size() * sizeof(Vertex);
      if (vertexBufferSize_) {
         vertexBuffer_ = gRDI->createVertexBuffer(vertexBufferSize_, verts.data());
      }
      _bLocalBox.min.x = _bLocalBox.min.y = _bLocalBox.min.z = FLT_MAX;
      _bLocalBox.max.x = _bLocalBox.max.y = _bLocalBox.max.z = FLT_MIN;
      for (const auto& v : verts) {
         _bLocalBox.min.x = std::min(_bLocalBox.min.x, v.position[0]);
         _bLocalBox.min.y = std::min(_bLocalBox.min.y, v.position[1]);
         _bLocalBox.min.z = std::min(_bLocalBox.min.z, v.position[2]);
         _bLocalBox.max.x = std::max(_bLocalBox.max.x, v.position[0]);
         _bLocalBox.max.y = std::max(_bLocalBox.max.y, v.position[1]);
         _bLocalBox.max.z = std::max(_bLocalBox.max.z, v.position[2]);
      }
   } else {
      _bLocalBox.clear();
   }
   markDirty();
}

void DebugShapesNode::decode(const protocol::shapelist &shapes)
{
   clear();
   for (int i = 0; i < shapes.box_size(); i++) {
      decode_box(shapes.box(i));
   }
   for (int i = 0; i < shapes.line_size(); i++) {
      decode_line(shapes.line(i));
   }
   for (int i = 0; i < shapes.quad_size(); i++) {
      decode_quad(shapes.quad(i));
   }
   for (int i = 0; i < shapes.coords_size(); i++) {
      decode_coord(shapes.coords(i));
   }
   for (auto& region : shapes.region()) {
      decode_region(region);
   }
   createBuffers();
}

void DebugShapesNode::decode_line(const protocol::line &line)
{
   csg::Point3f p0(line.p0()), p1(line.p1());
   csg::Color4 color(0xff, 0xff, 0, 0xff);

   if (line.has_color()) {
      color.LoadValue(line.color());
   }
   add_line(p0, p1, color);
}

void DebugShapesNode::decode_quad(const protocol::quad &quad)
{
   const csg::Point3f points[] = {
      csg::Point3f(quad.p0()),
      csg::Point3f(quad.p1()),
      csg::Point3f(quad.p2()),
      csg::Point3f(quad.p3()),
      csg::Point3f(quad.p0()),
   };
   csg::Color4 color(0xff, 0xff, 0, 0xff);
   if (quad.has_color()) {
      color.LoadValue(quad.color());
   }
#if 0
   add_line(points[0], points[1], c);
   add_line(points[1], points[2], c);
   add_line(points[2], points[3], c);
   add_line(points[3], points[4], c);
#else
   add_triangle(points, color);
   add_triangle(points + 2, color);
#endif
}

void DebugShapesNode::decode_coord(const protocol::coord &coord)
{
   const csg::Point3f points[] = {
      csg::Point3f((float)coord.x() - 0.5f, (float)coord.y(), (float)coord.z() - 0.5f),
      csg::Point3f((float)coord.x() - 0.5f, (float)coord.y(), (float)coord.z() + 0.5f),
      csg::Point3f((float)coord.x() + 0.5f, (float)coord.y(), (float)coord.z() + 0.5f),
      csg::Point3f((float)coord.x() + 0.5f, (float)coord.y(), (float)coord.z() - 0.5f),
      csg::Point3f((float)coord.x() - 0.5f, (float)coord.y(), (float)coord.z() - 0.5f),
   };
   csg::Color4 color(0xff, 0xff, 0, 0xff);
   if (coord.has_color()) {
      color.LoadValue(coord.color());
   }
#if 0
   add_line(points[0], points[1], c);
   add_line(points[1], points[2], c);
   add_line(points[2], points[3], c);
   add_line(points[3], points[4], c);
#else
   add_triangle(points, color);
   add_triangle(points + 2, color);
#endif
}

void DebugShapesNode::add_line(const csg::Point3f &p0, const csg::Point3f &p1, const csg::Color4 &c)
{
   lines_.push_back(Vertex(p0, c));
   lines_.push_back(Vertex(p1, c));
}


void DebugShapesNode::decode_box(const protocol::box &box)
{
   csg::Cube3f aabb(csg::Point3f(box.minimum()), csg::Point3f(box.maximum()));
   csg::Color4 color(0xff, 0xff, 0, 0xff);
   if (box.has_color()) {
      color.LoadValue(box.color());
   }
   add_aabb(aabb, color);
}

void DebugShapesNode::decode_region(const protocol::region3i &region)
{
   csg::Region3 rgn(region);
   csg::Color4 color(0xff, 0xff, 0, 0xff);
   if (region.has_color()) {
      color.LoadValue(region.color());
   }
   add_region(rgn, color);
}


void DebugShapesNode::add_aabb(const csg::Cube3f& aabb, const csg::Color4& color)
{  
   csg::Point3f a(aabb.GetMin()), b(aabb.GetMax());
   csg::Point3f bot_tris[] = {
      csg::Point3f(a.x, a.y, a.z),
      csg::Point3f(b.x, a.y, a.z),
      csg::Point3f(a.x, a.y, b.z),
      csg::Point3f(b.x, a.y, b.z)
   };
   csg::Point3f bot[] = {
      csg::Point3f(a.x, a.y, a.z),
      csg::Point3f(b.x, a.y, a.z),
      csg::Point3f(b.x, a.y, b.z),
      csg::Point3f(a.x, a.y, b.z)
   };
   csg::Point3f top[] = {
      csg::Point3f(a.x, b.y, a.z),
      csg::Point3f(b.x, b.y, a.z),
      csg::Point3f(b.x, b.y, b.z),
      csg::Point3f(a.x, b.y, b.z)
   };
   csg::Point3f left[] = {
      csg::Point3f(a.x, a.y, a.z),
      csg::Point3f(a.x, a.y, b.z),
      csg::Point3f(a.x, b.y, b.z),
      csg::Point3f(a.x, b.y, a.z)
   };
   csg::Point3f right[] = {
      csg::Point3f(b.x, a.y, a.z),
      csg::Point3f(b.x, a.y, b.z),
      csg::Point3f(b.x, b.y, b.z),
      csg::Point3f(b.x, b.y, a.z)
   };
   csg::Point3f front[] = {
      csg::Point3f(a.x, a.y, a.z),
      csg::Point3f(b.x, a.y, a.z),
      csg::Point3f(b.x, b.y, a.z),
      csg::Point3f(a.x, b.y, a.z)
   };
   csg::Point3f back[] = {
      csg::Point3f(a.x, a.y, b.z),
      csg::Point3f(b.x, a.y, b.z),
      csg::Point3f(b.x, b.y, b.z),
      csg::Point3f(a.x, b.y, b.z)
   };

   add_quad(bot, color);
   add_quad(top, color);
   add_quad(left, color);
   add_quad(right, color);
   add_quad(front, color);
   add_quad(back, color);

#if 0
   color.a *= 0.5;
   add_triangle(bot_tris, color);
   add_triangle(bot_tris + 1, color);
#endif
}

void DebugShapesNode::add_region(const csg::Region3& rgn, const csg::Color4& color)
{
   for (const auto &c : rgn) {
      // xxx: this is in no way thread safe! (see SH-8)
      static const float offset[] = { 0.5f, 0.0f, 0.5f };
      csg::Point3f min, max;
      for (int i = 0; i < 3; i++) {
         min[i] = (float)c.GetMin()[i] - offset[i];
         max[i] = (float)c.GetMax()[i] - offset[i];
      }
      add_aabb(csg::Cube3f(min, max), color);
   }
}

void DebugShapesNode::add_quad_xz(const csg::Point3f &p0, const csg::Point3f &p1, const csg::Color4 &color)
{
   csg::Point3f quad[] = {
      csg::Point3f(p0.x - 0.5f, p0.y - 0.5f, p0.z),
      csg::Point3f(p1.x - 0.5f, p0.y - 0.5f, p0.z),
      csg::Point3f(p1.x - 0.5f, p0.y - 0.5f, p1.z),
      csg::Point3f(p0.x - 0.5f, p0.y - 0.5f, p1.z)
   };
   add_quad(quad, color);
}

void DebugShapesNode::add_quad(const csg::Point3f points[4], const csg::Color4& color)
{
   for (int i = 0; i < 4; i++) {
      add_line(points[i], points[(i+1) % 4], color);
   }
}

void DebugShapesNode::add_triangle(const csg::Point3f points[3], const csg::Color4& color)
{
   for (int i = 0; i < 3; i++) {
      triangles_.push_back(Vertex(points[i], color));
   }
}

bool DebugShapesNode::empty()
{
   return triangles_.empty() && lines_.empty();
}

void DebugShapesNode::onFinishedUpdate()
{
   _bBox = _bLocalBox;
   _bBox.transform(_absTrans);
}
