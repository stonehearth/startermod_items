#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "egMaterial.h"
#include "egCamera.h"

#if defined(ASSERT)
#  undef ASSERT
#endif

#include "radiant.h"
#include "math3d.h"
#include "debug_shapes.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;

radiant::uint32 DebugShapesNode::vertexLayout;
MaterialResource* DebugShapesNode::material;

DebugShapesNode::DebugShapesNode(const DebugShapesTpl &terrainTpl) :
   SceneNode(terrainTpl),
   vertexBuffer_(0),
   vertexBufferSize_(0)
{
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
         if (!DebugShapesNode::material->isOfClass(theClass)) {
            continue;
         }
		   if (!Modules::renderer().setMaterial(DebugShapesNode::material, shaderContext)) {
            continue;
         }
         first = false;
      }
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(-1, -1);
      debugShapes->render();
   }
   if (!first) {
      glDisable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0, 0);
   }
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
   int start = 0, last = -1;

   std::vector<Vertex> verts;
   if (!lines_.empty()) {
      start = last + 1;
      last = start + lines_.size() - 1;
      std::copy(lines_.begin(), lines_.end(), std::back_inserter(verts));
      primitives_.push_back(Primitives(PRIM_LINES, start, last));
   }
   if (!triangles_.empty()) {
      start = last + 1;
      last = start + triangles_.size() - 1;
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
   math3d::point3 p0(line.p0()), p1(line.p1());
   math3d::color4 color(0xff, 0xff, 0, 0xff);

   if (line.has_color()) {
      color.LoadValue(line.color());
   }
   add_line(p0, p1, color);
}

void DebugShapesNode::decode_quad(const protocol::quad &quad)
{
   const math3d::point3 points[] = {
      math3d::point3(quad.p0()),
      math3d::point3(quad.p1()),
      math3d::point3(quad.p2()),
      math3d::point3(quad.p3()),
      math3d::point3(quad.p0()),
   };
   math3d::color4 color(0xff, 0xff, 0, 0xff);
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
   const math3d::point3 points[] = {
      math3d::point3((float)coord.x() - 0.5f, (float)coord.y(), (float)coord.z() - 0.5f),
      math3d::point3((float)coord.x() - 0.5f, (float)coord.y(), (float)coord.z() + 0.5f),
      math3d::point3((float)coord.x() + 0.5f, (float)coord.y(), (float)coord.z() + 0.5f),
      math3d::point3((float)coord.x() + 0.5f, (float)coord.y(), (float)coord.z() - 0.5f),
      math3d::point3((float)coord.x() - 0.5f, (float)coord.y(), (float)coord.z() - 0.5f),
   };
   math3d::color4 color(0xff, 0xff, 0, 0xff);
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

void DebugShapesNode::add_line(const math3d::point3 &p0, const math3d::point3 &p1, const math3d::color4 &c)
{
   lines_.push_back(Vertex(p0, c));
   lines_.push_back(Vertex(p1, c));
}


void DebugShapesNode::decode_box(const protocol::box &box)
{
   math3d::aabb aabb(math3d::point3(box.minimum()), math3d::point3(box.maximum()));
   math3d::color4 color(0xff, 0xff, 0, 0xff);
   if (box.has_color()) {
      color.LoadValue(box.color());
   }
   add_aabb(aabb, color);
}

void DebugShapesNode::decode_region(const protocol::region &region)
{
   csg::Region3 rgn(region);
   math3d::color4 color(0xff, 0xff, 0, 0xff);
   if (region.has_color()) {
      color.LoadValue(region.color());
   }
   add_region(rgn, color);
}


void DebugShapesNode::add_aabb(const math3d::aabb& aabb, const math3d::color4& color)
{  
   math3d::point3 a(aabb._min), b(aabb._max);
   math3d::point3 bot_tris[] = {
      math3d::point3(a.x, a.y, a.z),
      math3d::point3(b.x, a.y, a.z),
      math3d::point3(a.x, a.y, b.z),
      math3d::point3(b.x, a.y, b.z)
   };
   math3d::point3 bot[] = {
      math3d::point3(a.x, a.y, a.z),
      math3d::point3(b.x, a.y, a.z),
      math3d::point3(b.x, a.y, b.z),
      math3d::point3(a.x, a.y, b.z)
   };
   math3d::point3 top[] = {
      math3d::point3(a.x, b.y, a.z),
      math3d::point3(b.x, b.y, a.z),
      math3d::point3(b.x, b.y, b.z),
      math3d::point3(a.x, b.y, b.z)
   };
   math3d::point3 left[] = {
      math3d::point3(a.x, a.y, a.z),
      math3d::point3(a.x, a.y, b.z),
      math3d::point3(a.x, b.y, b.z),
      math3d::point3(a.x, b.y, a.z)
   };
   math3d::point3 right[] = {
      math3d::point3(b.x, a.y, a.z),
      math3d::point3(b.x, a.y, b.z),
      math3d::point3(b.x, b.y, b.z),
      math3d::point3(b.x, b.y, a.z)
   };
   math3d::point3 front[] = {
      math3d::point3(a.x, a.y, a.z),
      math3d::point3(b.x, a.y, a.z),
      math3d::point3(b.x, b.y, a.z),
      math3d::point3(a.x, b.y, a.z)
   };
   math3d::point3 back[] = {
      math3d::point3(a.x, a.y, b.z),
      math3d::point3(b.x, a.y, b.z),
      math3d::point3(b.x, b.y, b.z),
      math3d::point3(a.x, b.y, b.z)
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

void DebugShapesNode::add_region(const csg::Region3& rgn, const math3d::color4& color)
{
   for (const auto &c : rgn) {
      math3d::aabb a;
      static const float offset[] = { 0.5f, 0.0f, 0.5f };
      for (int i = 0; i < 3; i++) {
         a._min[i] = (float)c.GetMin()[i] - offset[i];
         a._max[i] = (float)c.GetMax()[i] - offset[i];
      }
      add_aabb(a, color);
   }
}

void DebugShapesNode::add_quad_xz(const math3d::point3 &p0, const math3d::point3 &p1, const math3d::color4 &color)
{
   math3d::point3 quad[] = {
      math3d::point3(p0.x - 0.5f, p0.y - 0.5f, p0.z),
      math3d::point3(p1.x - 0.5f, p0.y - 0.5f, p0.z),
      math3d::point3(p1.x - 0.5f, p0.y - 0.5f, p1.z),
      math3d::point3(p0.x - 0.5f, p0.y - 0.5f, p1.z)
   };
   add_quad(quad, color);
}

void DebugShapesNode::add_quad(const math3d::point3 points[4], const math3d::color4& color)
{
   for (int i = 0; i < 4; i++) {
      add_line(points[i], points[(i+1) % 4], color);
   }
}

void DebugShapesNode::add_triangle(const math3d::point3 points[3], const math3d::color4& color)
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
