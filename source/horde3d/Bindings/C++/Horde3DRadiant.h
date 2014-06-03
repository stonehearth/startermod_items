#pragma once

#include "Horde3D.h"
#include "csg/point.h"
#include "csg/color.h"
#include "csg/meshtools.h"
#include "protocols/radiant.pb.h"

const int H3DRadiant_NodeType_DebugShape = 1000;

DLL H3DNode h3dRadiantAddDebugShapes(H3DNode parent, const char* name);
DLL bool h3dRadiantDecodeDebugShapes(H3DNode node, const ::radiant::protocol::shapelist &shapes);
DLL bool h3dRadiantClearDebugShape(H3DNode node);
DLL bool h3dRadiantAddDebugBox(H3DNode node, const ::radiant::csg::Cube3f& box, const ::radiant::csg::Color4& color);
DLL bool h3dRadiantAddDebugRegion(H3DNode node, const ::radiant::csg::Region3& rgn, ::radiant::csg::Point3f const& offset, const ::radiant::csg::Color4& color);
DLL bool h3dRadiantAddDebugQuadXZ(H3DNode node, const ::radiant::csg::Point3f& p0, const ::radiant::csg::Point3f& p1, const ::radiant::csg::Color4& color);
DLL bool h3dRadiantAddDebugLine(H3DNode node, const ::radiant::csg::Point3f& p0, const ::radiant::csg::Point3f& p1, const ::radiant::csg::Color4& color);
DLL bool h3dRadiantCommitDebugShape(H3DNode node);
DLL H3DNode h3dRadiantAddCubemitterNode(H3DNode parent, const char* nam, H3DRes cubeRes);
DLL void h3dRadiantAdvanceCubemitterTime(float timeDelta);
DLL void h3dRadiantStopCubemitterNode(H3DNode node);
DLL void h3dRadiantAdvanceAnimatedLightTime(float timeDelta);
DLL H3DNode h3dRadiantAddAnimatedLightNode(H3DNode parent, const char* nam, H3DRes animatedLightRes);

#include "../extensions/stockpile/stockpile_node.h"
#include "../extensions/stockpile/decal_node.h"
#include "../extensions/stockpile/toast_node.h"

H3DNode h3dRadiantCreateStockpileNode(H3DNode parent, std::string const& name);
bool h3dRadiantResizeStockpileNode(H3DNode node, int width, int height);

std::pair<H3DNode, ::radiant::horde3d::DecalNode*> h3dRadiantCreateDecalNode(H3DNode parent, std::string const& name, std::string const& material);
::radiant::horde3d::ToastNode* h3dRadiantCreateToastNode(H3DNode parent, std::string const& name);
