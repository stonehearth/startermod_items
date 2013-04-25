#include "utPlatform.h"
#include "egCom.h"
#include "egModules.h"
#include "egRenderer.h"
#include "extension.h"
#include "radiant.h"
#include "debug_shapes/debug_shapes.h"
#include "stockpile/decal_node.h"
#include "stockpile/stockpile_node.h"
#include "stockpile/toast_node.h"
#include "stockpile/terrain_node.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;

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
   if (!TerrainNode::InitExtension()) {
      return false;
   }

	Modules::sceneMan().registerType(SNT_DebugShapesNode, "DebugShapes",
		                              DebugShapesNode::parsingFunc,
                                    DebugShapesNode::factoryFunc,
                                    DebugShapesNode::renderFunc);

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

   return true;
}

void Extension::release()
{
   // xxx: nuke the material
}

std::pair<H3DNode, ::radiant::horde3d::StockpileNode*> h3dRadiantCreateStockpileNode(H3DNode parent, std::string name)
{
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantCreateStockpileNode", std::make_pair(0, nullptr));
	
	Modules::log().writeInfo( "Adding Stockpile node '%s'", name.c_str() );
	
	StockpileNode* sn = (StockpileNode*)Modules::sceneMan().findType(SNT_StockpileNode)->factoryFunc(StockpileTpl(name));
	H3DNode node = Modules::sceneMan().addNode(sn, *parentNode);

   return std::make_pair(node, sn);
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

::radiant::horde3d::TerrainNode* h3dRadiantCreateTerrainNode(H3DNode parent, std::string name, int type, csg::mesh_tools::mesh const& m)
{
	SceneNode *parentNode = Modules::sceneMan().resolveNodeHandle( parent );
	APIFUNC_VALIDATE_NODE(parentNode, "h3dRadiantCreateTerrainNode", nullptr);
	
	Modules::log().writeInfo( "Adding Terrain node '%s'", name.c_str() );
	
	TerrainNode* sn = new TerrainNode(type, m);
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
   //tpl.material = Modules::resMan().

	SceneNode *sn = Modules::sceneMan().findType(SNT_DebugShapesNode)->factoryFunc(tpl);
	return Modules::sceneMan().addNode(sn, *parentNode);
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

DLL bool h3dRadiantAddDebugBox(H3DNode node, const math3d::aabb& box, const math3d::color4& color)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantAddDebugBox", false);

   static_cast<DebugShapesNode*>(sn)->add_aabb(box, color);
   return true;
}

DLL bool h3dRadiantAddDebugRegion(H3DNode node, const csg::Region3& rgn, const math3d::color4& color)
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

DLL bool h3dRadiantAddDebugQuadXZ(H3DNode node, const ::radiant::math3d::point3& p0, const ::radiant::math3d::point3& p1, const ::radiant::math3d::color4& color)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantAddDebugBox", false);

   static_cast<DebugShapesNode*>(sn)->add_quad_xz(p0, p1, color);
   return true;
}

DLL bool h3dRadiantAddDebugLine(H3DNode node, const ::radiant::math3d::point3& p0, const ::radiant::math3d::point3& p1, const ::radiant::math3d::color4& color)
{
	SceneNode *sn = Modules::sceneMan().resolveNodeHandle(node);
   APIFUNC_VALIDATE_NODE_TYPE(sn, SNT_DebugShapesNode, "h3dRadiantAddDebugBox", false);

   static_cast<DebugShapesNode*>(sn)->add_line(p0, p1, color);
   return true;
}
