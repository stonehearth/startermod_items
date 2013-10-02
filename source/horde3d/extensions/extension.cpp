#include "utPlatform.h"
#include "egCom.h"
#include "egModules.h"
#include "egRenderer.h"
#include "extension.h"
#include "radiant.h"
#include "animatedlight/animatedlight.h"
#include "cubemitter/cubemitter.h"
#include "debug_shapes/debug_shapes.h"
#include "stockpile/decal_node.h"
#include "stockpile/stockpile_node.h"
#include "stockpile/toast_node.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;

radiant::uint32 Extension::_cubeVBO;
radiant::uint32 Extension::_cubeIdxBuf;
radiant::uint32 Extension::_vlCube;

Extension::Extension()
{
}

Extension::~Extension()
{
}

const char *Extension::getName()
{
   return "Radiant Debug Shapes";
}

bool Extension::init()
{
   if (!StockpileNode::InitExtension()) {
      return false;
   }
   if (!DecalNode::InitExtension()) {
      return false;
   }
   if (!ToastNode::InitExtension()) {
      return false;
   }

	Modules::sceneMan().registerType(SNT_DebugShapesNode, "DebugShapes",
		                              DebugShapesNode::parsingFunc,
                                    DebugShapesNode::factoryFunc,
                                    DebugShapesNode::renderFunc);
   Modules::sceneMan().registerType(SNT_CubemitterNode, "Cubemitter",
		                              CubemitterNode::parsingFunc,
                                    CubemitterNode::factoryFunc,
                                    CubemitterNode::renderFunc);
   Modules::sceneMan().registerType(SNT_AnimatedLightNode, "AnimatedLight",
                                    AnimatedLightNode::parsingFunc,
                                    AnimatedLightNode::factoryFunc,
                                    0x0);

   Modules::resMan().registerType(RT_CubemitterResource, "Cubemitter", 0x0, 0x0, 
                                    CubemitterResource::factoryFunc);
   Modules::resMan().registerType(RT_AnimatedLightResource, "AnimatedLight", 0x0, 0x0,
                                    AnimatedLightResource::factoryFunc);

	VertexLayoutAttrib attribs[2] = {
		"vertPos",     0, 3, 0,
		"inputColor",  0, 3, 12,
	};
	DebugShapesNode::vertexLayout = gRDI->registerVertexLayout(2, attribs);

   H3DRes mat = h3dAddResource(H3DResTypes::Material, "materials/debug_shape.material.xml", 0);
	Resource *matRes =  Modules::resMan().resolveResHandle(mat);
	if (matRes == 0x0 || matRes->getType() != ResourceTypes::Material ) {
      return 0;
   }
   DebugShapesNode::material = (MaterialResource *)matRes;


   const int numCubeAttributes = 3;
   VertexLayoutAttrib attribsCube[numCubeAttributes] = {
      {"vertPos", 0, 3, 0},
      {"particleWorldMatrix", 1, 4 * 4, 0},
      {"cubeColor", 1, 4, 64},
	};
   VertexDivisorAttrib divisors[numCubeAttributes] = {
      0, // vertPos is just a normal attribute
      1, // The world-matrix is a per-instance field.
      1  // The color is a per-instance field.
   };
	_vlCube = gRDI->registerVertexLayout( numCubeAttributes, attribsCube, divisors );

   // Create cube geometry array
	CubeVert cvs[8];
   cvs[0] = CubeVert( -0.5, -0.5, 0.5);
	cvs[1] = CubeVert( 0.5, -0.5,  0.5);
	cvs[2] = CubeVert( 0.5,  0.5, 0.5);
	cvs[3] = CubeVert( -0.5,  0.5,  0.5);
	cvs[4] = CubeVert(  -0.5, -0.5, -0.5);
	cvs[5] = CubeVert(  0.5, -0.5,  -0.5);
	cvs[6] = CubeVert(  0.5,  0.5, -0.5);
	cvs[7] = CubeVert(  -0.5,  0.5,  -0.5);
	
	_cubeVBO = gRDI->createVertexBuffer( 8 * sizeof( CubeVert ), (float *)cvs );
	uint16 cubeInds[36] = {
		0, 1, 2, 2, 3, 0,   1, 5, 6, 6, 2, 1,   5, 4, 7, 7, 6, 5,
		4, 0, 3, 3, 7, 4,   3, 2, 6, 6, 7, 3,   4, 5, 1, 1, 0, 4
	};
   _cubeIdxBuf = gRDI->createIndexBuffer(36 * sizeof(uint16), (void *)cubeInds);


   return true;
}

void Extension::release()
{
   gRDI->destroyBuffer( _cubeVBO );
   gRDI->destroyBuffer( _cubeIdxBuf );

   // xxx: nuke the material
}

H3DNode h3dRadiantCreateStockpileNode(H3DNode parent, std::string name)
{
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantCreateStockpileNode", 0);
	
	Modules::log().writeInfo( "Adding Stockpile node '%s'", name.c_str() );
	
	StockpileNode* sn = (StockpileNode*)Modules::sceneMan().findType(SNT_StockpileNode)->factoryFunc(StockpileTpl(name));
	H3DNode node = Modules::sceneMan().addNode(sn, *parentNode);

   return node;
}

bool h3dRadiantResizeStockpileNode(H3DNode node, int width, int height)
{
	SceneNode *sceneNode = Modules::sceneMan().resolveNodeHandle( node );
	APIFUNC_VALIDATE_NODE(sceneNode, "h3dRadiantResizeStockpileNode", false);

   APIFUNC_VALIDATE_NODE_TYPE(sceneNode, SNT_StockpileNode, "h3dRadiantResizeStockpileNode", false );

   csg::Cube3 bounds(csg::Point3::zero, csg::Point3(width, 0, height));
   ((StockpileNode*)sceneNode)->UpdateShape(bounds);
   return true;
}

::radiant::horde3d::ToastNode* h3dRadiantCreateToastNode(H3DNode parent, std::string name)
{
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantCreateToastNode", nullptr);
	
	Modules::log().writeInfo( "Adding Toast node '%s'", name.c_str() );
	
	ToastNode* sn = (ToastNode*)Modules::sceneMan().findType(SNT_ToastNode)->factoryFunc(ToastTpl(name));
	H3DNode node = Modules::sceneMan().addNode(sn, *parentNode);

   sn->SetNode(node);
   return sn;
}

std::pair<H3DNode, ::radiant::horde3d::DecalNode*> h3dRadiantCreateDecalNode(H3DNode parent, std::string name, std::string material)
{
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantCreateDecalNode", std::make_pair(0, nullptr));
	
	Modules::log().writeInfo( "Adding Decal node '%s'", name.c_str() );
	
	DecalNode* sn = (DecalNode*)Modules::sceneMan().findType(SNT_DecalNode)->factoryFunc(DecalTpl(name));
   if (!sn->SetMaterial(material)) {
      return std::make_pair(0, nullptr);
   }

	H3DNode node = Modules::sceneMan().addNode(sn, *parentNode);

   return std::make_pair(node, sn);
}

DLL H3DNode h3dRadiantAddDebugShapes(H3DNode parent, const char* nam)
{
   std::string name(nam);
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantAddDebugShapes", 0);
	
	Modules::log().writeInfo( "Adding Debug Shape node '%s'", name.c_str() );
	
	DebugShapesTpl tpl(name);

	SceneNode *sn = Modules::sceneMan().findType(SNT_DebugShapesNode)->factoryFunc(tpl);
	return Modules::sceneMan().addNode(sn, *parentNode);
}

DLL H3DNode h3dRadiantAddCubemitterNode(H3DNode parent, const char* nam, H3DRes cubemitter, H3DRes mat)
{
   std::string name(nam);
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantAddCubemitterNode", 0);
	
   CubemitterResource *cubeRes = (CubemitterResource *)Modules::resMan().resolveResHandle(cubemitter);
   MaterialResource *matRes = (MaterialResource *)Modules::resMan().resolveResHandle(mat);
   
   APIFUNC_VALIDATE_RES_TYPE(matRes, ResourceTypes::Material, "h3dRadiantAddCubemitterNode", 0);
   APIFUNC_VALIDATE_RES_TYPE(cubeRes, RT_CubemitterResource, "h3dRadiantAddCubemitterNode", 0);

   CubemitterNodeTpl tpl(name, cubeRes, matRes);

	SceneNode *sn = Modules::sceneMan().findType(SNT_CubemitterNode)->factoryFunc(tpl);
	return Modules::sceneMan().addNode(sn, *parentNode);
}

DLL void h3dRadiantAdvanceCubemitterTime(float timeDelta) {
   int numNodes = h3dFindNodes(H3DRootNode, "", SNT_CubemitterNode);
   while (numNodes > 0) {
      H3DNode n = h3dGetNodeFindResult(--numNodes);
      SceneNode *sn = Modules::sceneMan().resolveNodeHandle( n );
      APIFUNC_VALIDATE_NODE_TYPE( sn, SNT_CubemitterNode, "h3dAdvanceCubemitterTime", APIFUNC_RET_VOID );
      CubemitterNode * cn = (CubemitterNode *)sn;
      if (cn->hasFinished())
      {
         h3dRemoveNode(n);
      } else
      {
         cn->advanceTime( timeDelta );
      }
   }
}

DLL void h3dRadiantStopCubemitterNode(H3DNode n) {
   SceneNode *sn = Modules::sceneMan().resolveNodeHandle(n);
   APIFUNC_VALIDATE_NODE_TYPE( sn, SNT_CubemitterNode, "h3dAdvanceCubemitterTime", APIFUNC_RET_VOID );
   CubemitterNode *cn = (CubemitterNode *)sn;      
   // Do a soft-shutdown of the particle system.  We'll reap the node during update, once all the
   // cubes are gone.
   cn->stop();
}

DLL bool h3dRadiantClearDebugShape(H3DNode node)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantClearDebugShape", false);

   static_cast<DebugShapesNode*>(sn)->clear();
   return true;
}

DLL bool h3dRadiantCommitDebugShape(H3DNode node)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantCommitDebugShape", false);

   static_cast<DebugShapesNode*>(sn)->createBuffers();
   return true;
}

DLL bool h3dRadiantAddDebugBox(H3DNode node, const csg::Cube3f& box, const csg::Color4& color)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantAddDebugBox", false);

   static_cast<DebugShapesNode*>(sn)->add_aabb(box, color);
   return true;
}

DLL bool h3dRadiantAddDebugRegion(H3DNode node, const csg::Region3& rgn, const csg::Color4& color)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantAddDebugRegion", false);

   static_cast<DebugShapesNode*>(sn)->add_region(rgn, color);
   return true;
}


DLL bool h3dRadiantDecodeDebugShapes(H3DNode node, const protocol::shapelist &shapes)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantDecodeDebugShapes", false);

   static_cast<DebugShapesNode*>(sn)->decode(shapes);
   return true;
}

DLL bool h3dRadiantAddDebugQuadXZ(H3DNode node, const ::radiant::csg::Point3f& p0, const ::radiant::csg::Point3f& p1, const ::radiant::csg::Color4& color)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantAddDebugBox", false);

   static_cast<DebugShapesNode*>(sn)->add_quad_xz(p0, p1, color);
   return true;
}

DLL bool h3dRadiantAddDebugLine(H3DNode node, const ::radiant::csg::Point3f& p0, const ::radiant::csg::Point3f& p1, const ::radiant::csg::Color4& color)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantAddDebugBox", false);

   static_cast<DebugShapesNode*>(sn)->add_line(p0, p1, color);
   return true;
}

DLL H3DNode h3dRadiantAddAnimatedLightNode(H3DNode parent, const char* nam, H3DRes animatedLightRes, H3DRes mat)
{
   std::string name(nam);
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantAddAnimatedLight", 0);

   AnimatedLightResource *lightRes = (AnimatedLightResource *)Modules::resMan().resolveResHandle(animatedLightRes);
   MaterialResource *matRes = (MaterialResource *)Modules::resMan().resolveResHandle(mat);
   
   APIFUNC_VALIDATE_RES_TYPE(matRes, ResourceTypes::Material, "h3dRadiantAddAnimatedLight", 0);
   APIFUNC_VALIDATE_RES_TYPE(lightRes, RT_AnimatedLightResource, "h3dRadiantAddAnimatedLight", 0);

   AnimatedLightNodeTpl tpl(name, lightRes, matRes);

	SceneNode *sn = Modules::sceneMan().findType(SNT_AnimatedLightNode)->factoryFunc(tpl);
   H3DNode ln = Modules::sceneMan().addNode(sn, *parentNode);
   AnimatedLightNode *an = (AnimatedLightNode *)Modules::sceneMan().resolveNodeHandle( ln );
   an->init();
	return ln;
}

DLL void h3dRadiantAdvanceAnimatedLightTime(float timeDelta) {
   int numNodes = h3dFindNodes(H3DRootNode, "", SNT_AnimatedLightNode);
   while (numNodes > 0) {
      H3DNode n = h3dGetNodeFindResult(--numNodes);
      SceneNode *sn = Modules::sceneMan().resolveNodeHandle( n );
      APIFUNC_VALIDATE_NODE_TYPE( sn, SNT_AnimatedLightNode, "h3dRadiantAdvanceAnimatedLightTime", APIFUNC_RET_VOID );
      AnimatedLightNode * an = (AnimatedLightNode *)sn;
      if (an->hasFinished())
      {
         h3dRemoveNode(n);
      } else
      {
         an->advanceTime( timeDelta );
      }
   }
}
