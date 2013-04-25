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
#include "terrain_node.h"
#include "csg/meshtools.h"
#include "csg/cube.h"
#include "om/components/terrain.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;
using namespace ::Horde3D;


unsigned int TerrainNode::vertexLayout;
Horde3D::MaterialResource* TerrainNode::material_;
std::unordered_map<int, Horde3D::MaterialResource*> TerrainNode::material_map_;

TerrainNode::TerrainNode(int type, csg::mesh_tools::mesh const& m) :
   SceneNode(Horde3D::SceneNodeTpl(SNT_TerrainNode, "terrain node")),
   vertexBuffer_(0),
   indexBuffer_(0),
   vertexCount_(0),
   indexCount_(0),
   terrain_type_(type)
{
   _renderable = true;
   CreateRenderMesh(m);
}

TerrainNode::~TerrainNode()
{
}

bool TerrainNode::InitExtension()
{
	Modules::sceneMan().registerType(SNT_TerrainNode, "Terrain",
		                              nullptr,
                                    nullptr,
                                    TerrainNode::renderFunc);

	VertexLayoutAttrib attribs[] = {
      { "vertPos",     0, 3, 0 }, 
      { "normal",      0, 3, 12 },
      { "color",       0, 3, 24 },
	};
	TerrainNode::vertexLayout = gRDI->registerVertexLayout(ARRAY_SIZE(attribs), attribs);

   auto load_material = [=](const char* name) -> Horde3D::MaterialResource* {
      H3DRes mat = h3dAddResource(H3DResTypes::Material, name, 0);
	   Resource *matRes =  Modules::resMan().resolveResHandle(mat);
	   if (matRes == 0x0 || matRes->getType() != ResourceTypes::Material ) {
         return 0;
      }
      h3dutLoadResourcesFromDisk("horde");
      return (MaterialResource *)matRes;
   };
   material_ = load_material("terrain/default_material.xml");
   material_map_[om::Terrain::Bedrock] = load_material("terrain/bedrock_material.xml");
   material_map_[om::Terrain::Topsoil] = load_material("terrain/topsoil_material.xml");

   return true;
}

void TerrainNode::renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                             const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet)
{
   bool offsetSet = false;
   for (const auto &entry : Modules::sceneMan().getRenderableQueue()) {
      if (entry.type != SNT_TerrainNode) {
         continue;
      }
      TerrainNode *terrain = (TerrainNode *)entry.node;

      // xxx: check out Renderer::drawMeshes
      // there's a ton of logic to do things like occlusion culling, query objects, etc.  does that logic
      // have to be duplicated here?  ug!
      auto i = material_map_.find(terrain->terrain_type_);
      MaterialResource* material = i != material_map_.end() ? i->second : material_;

      if (!debugView) {
         if (!material || !material->isOfClass(theClass)) {
            continue;
         }
         if (!Modules::renderer().setMaterial(material, shaderContext)) {
            continue;
         }
      } else {
			Vec4f color;
         int curLod = 0;
			if( curLod == 0 ) color = Vec4f( 0.5f, 0.75f, 1, 1 );
			else if( curLod == 1 ) color = Vec4f( 0.25f, 0.75, 0.75f, 1 );
			else if( curLod == 2 ) color = Vec4f( 0.25f, 0.75, 0.5f, 1 );
			else if( curLod == 3 ) color = Vec4f( 0.5f, 0.5f, 0.25f, 1 );
			else color = Vec4f( 0.75f, 0.5, 0.25f, 1 );

			Modules::renderer().setShaderComb( &Modules::renderer()._defColorShader );
			Modules::renderer().commitGeneralUniforms();
         gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &color.x );
      }
      terrain->render();
   }
}

void TerrainNode::render()
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

   gRDI->setIndexBuffer(indexBuffer_, IDXFMT_16);
   gRDI->setVertexBuffer(0, vertexBuffer_, 0, sizeof(csg::mesh_tools::vertex));
   gRDI->setVertexLayout(vertexLayout);
   gRDI->drawIndexed(PRIM_TRILIST, 0, indexCount_, 0, vertexCount_);

   Modules::stats().incStat(EngineStats::BatchCount, 1);
   Modules::stats().incStat(EngineStats::TriCount, indexCount_ / 3.0f);

   gRDI->setVertexLayout(0);
}

void TerrainNode::CreateRenderMesh(csg::mesh_tools::mesh const& m)
{
   mesh_ = m;

   // set the bounding box of the node
   _bLocalBox.min.x = m.bounds.GetMin().x;
   _bLocalBox.min.y = m.bounds.GetMin().y;
   _bLocalBox.min.z = m.bounds.GetMin().z;
   _bLocalBox.max.x = m.bounds.GetMax().x;
   _bLocalBox.max.y = m.bounds.GetMax().y;
   _bLocalBox.max.z = m.bounds.GetMax().z;

   // set index and vertex buffers
   if (!m.vertices.empty()) {
      vertexCount_ = m.vertices.size();
      vertexBuffer_ = gRDI->createVertexBuffer(vertexCount_ * sizeof(csg::mesh_tools::vertex), m.vertices.data());

      indexCount_ = m.indices.size();
      indexBuffer_ = gRDI->createIndexBuffer(indexCount_ * sizeof(radiant::uint16), m.indices.data());
   }
}

void TerrainNode::onFinishedUpdate()
{
   _bBox = _bLocalBox;

   Vec3f trans = _absTrans.getTrans();
   Vec3f scale = _absTrans.getScale();
   _absTrans = Matrix4f::TransMat(trans.x, trans.y, trans.z) * Matrix4f::ScaleMat(scale.x, scale.y, scale.z);
   _bBox.transform(_absTrans);
}

// xxx: we're stealing a lot from mesh node... perhaps we should just make a new MeshNode that has a vertex
// format much more tailored to our app?  No one's going to use that one, anyway...
bool TerrainNode::checkIntersection( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const
{
	if( !rayAABBIntersection( rayOrig, rayDir, _bBox.min, _bBox.max ) ) return false;
	
	// Transform ray to local space
	Matrix4f m = _absTrans.inverted();
	Vec3f orig = m * rayOrig;
	Vec3f dir = m * (rayOrig + rayDir) - orig;

	Vec3f nearestIntsPos = Vec3f( Math::MaxFloat, Math::MaxFloat, Math::MaxFloat );
	bool intersection = false;
	
	// Check triangles
   for (int i = 0; i < mesh_.indices.size(); i += 3) {
      Vec3f const* vert0 = (Vec3f const*)&mesh_.vertices[mesh_.indices[i]];
      Vec3f const* vert1 = (Vec3f const*)&mesh_.vertices[mesh_.indices[i+1]];
      Vec3f const* vert2 = (Vec3f const*)&mesh_.vertices[mesh_.indices[i+2]];

		if( rayTriangleIntersection( orig, dir, *vert0, *vert1, *vert2, intsPos ) ) {
			intersection = true;
			if( (intsPos - orig).length() < (nearestIntsPos - orig).length() ) {
				nearestIntsPos = intsPos;
            intsNorm = (*vert1 - *vert0).cross(*vert2 - *vert1);
         }
		}
   }

   intsNorm.normalize();
	intsPos = _absTrans * nearestIntsPos;
	
	return intersection;
}