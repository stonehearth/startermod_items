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
#include "decal_node.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;
using namespace ::Horde3D;

unsigned int DecalNode::vertexLayout;
MaterialResource* DecalNode::material;
::radiant::uint32 DecalNode::indexBuffer_;
::radiant::uint32 DecalNode::indexCount_;

DecalNode::DecalNode(const DecalTpl &terrainTpl) :
   SceneNode(terrainTpl),
   material_(nullptr),
   vertexBuffer_(0),
   vertexCount_(0)
{
   _renderable = true;
}

DecalNode::~DecalNode()
{
}

bool DecalNode::InitExtension()
{
	Modules::sceneMan().registerType(SNT_DecalNode, "Decal",
		                              DecalNode::parsingFunc,
                                    DecalNode::factoryFunc,
                                    DecalNode::renderFunc);

	VertexLayoutAttrib attribs[2] = {
		"vertPos",     0, 3, 0,
		"texCoords0",  0, 2, 12,
	};
	DecalNode::vertexLayout = gRDI->registerVertexLayout(2, attribs);


   std::vector<unsigned short> indices;

   indices.push_back(0);  // first triangle...
   indices.push_back(1);
   indices.push_back(2);
   indices.push_back(2);  // second triangle...
   indices.push_back(3);
   indices.push_back(0);

   indexCount_ = indices.size();
   indexBuffer_ = gRDI->createIndexBuffer(indexCount_ * sizeof(unsigned short), indices.data());

   return true;
}

SceneNodeTpl *DecalNode::parsingFunc(std::map<std::string, std::string> &attribs)
{
   return new DecalTpl("");
}

SceneNode *DecalNode::factoryFunc(const SceneNodeTpl &nodeTpl)
{
   if (nodeTpl.type != SNT_DecalNode) {
      return nullptr;
   }
   return new DecalNode(static_cast<const DecalTpl&>(nodeTpl));
}

void DecalNode::renderFunc(const std::string &shaderContext, const std::string &theClass, bool debugView,
                           const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet)
{
   bool offsetSet = false;
   for (const auto &entry : Modules::sceneMan().getRenderableQueue()) {
      if (entry.type != SNT_DecalNode) {
         continue;
      }
      DecalNode *decal = (DecalNode *)entry.node;

      MaterialResource* material = decal->GetMaterial();
      if (!material || !material->isOfClass(theClass)) {
         continue;
      }
		if (!Modules::renderer().setMaterial(material, shaderContext)) {
         continue;
      }
      if (!offsetSet) {
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(-1.0, -1.0);
         offsetSet = true;
      }
      decal->render();
   }
   if (offsetSet) {
      glDisable(GL_POLYGON_OFFSET_FILL);
   }
}

bool DecalNode::SetMaterial(std::string name)
{
   H3DRes mat = h3dAddResource(H3DResTypes::Material, name.c_str(), 0);

	Resource *matRes =  Modules::resMan().resolveResHandle(mat);
	if (matRes == 0x0 || matRes->getType() != ResourceTypes::Material ) {
      return false;
   }
   material_ = (MaterialResource *)matRes;
   return true;
}


void DecalNode::render()
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

void DecalNode::UpdateShape(const Vertex* verts, int vertexCount)
{
	gRDI->destroyBuffer(vertexBuffer_);
   vertexCount_ = vertexCount;
   if (vertexCount_) {
      vertexBuffer_ = gRDI->createVertexBuffer(vertexCount_ * sizeof(Vertex), STREAM, verts);
   } else {
      vertexBuffer_ = 0;
   }

   _bLocalBox.clear();
   if (vertexCount) {
      for (int i = 0; i < vertexCount; i++) {
         _bLocalBox.addPoint(Vec3f(verts[i].x, verts[i].y, verts[i].z));
      }
   }
}

void DecalNode::onFinishedUpdate()
{
   _bBox = _bLocalBox;
   _bBox.transform(_absTrans);
}
