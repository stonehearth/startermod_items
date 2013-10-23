#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include "egMaterial.h"
#include "egCamera.h"
#include "Horde3D.h"

#if defined(ASSERT)
#  undef ASSERT
#endif

#include "extension.h"
#include "rect.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;


SceneNodeTpl* ScreenSpaceNode::parsingFunc( std::map< std::string, std::string > &attribs )
{
   return nullptr;
}

SceneNode* ScreenSpaceNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SNT_ScreenSpaceNode ) return 0x0;

   return new ScreenSpaceNode( *(ScreenSpaceNodeTpl *)&nodeTpl);
}

ScreenSpaceNode::ScreenSpaceNode( const ScreenSpaceNodeTpl &screenSpaceTpl )
   : SceneNode(screenSpaceTpl)
{

}

ScreenSpaceNode::~ScreenSpaceNode()
{
   for (auto* shape : shapes_)
   {
      delete shape;
   }

   shapes_.clear();
}

ScreenSpaceRect* ScreenSpaceNode::addRect(int left, int right, int top, int bottom, csg::Color3& borderColor,
      int borderWidth, bool filled, csg::Color3& fillColor)
{
   ScreenSpaceRect* result = new ScreenSpaceRect(getHandle(), left, right, top, bottom);
   shapes_.emplace_back((ScreenSpaceShape*)result);

   return result;
}

void ScreenSpaceNode::init()
{

}
