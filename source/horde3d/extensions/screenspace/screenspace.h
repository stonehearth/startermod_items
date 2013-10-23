#ifndef _RADIANT_HORDE3D_EXTENSIONS_SCREENSPACE_H
#define _RADIANT_HORDE3D_EXTENSIONS_SCREENSPACE_H

#include "../../horde3d/Source/Horde3DEngine/egPrerequisites.h"
#include "../../horde3d/Source/Horde3DEngine/egScene.h"
#include "../extension.h"
#include "csg/color.h"
#include "namespace.h"
#include "Horde3d.h"

BEGIN_RADIANT_HORDE3D_NAMESPACE

struct ScreenSpaceNodeTpl : public Horde3D::SceneNodeTpl
{
   ScreenSpaceNodeTpl(const std::string &name) :
      SceneNodeTpl(SNT_ScreenSpaceNode, name)
   {
   }
};

// =================================================================================================

class ScreenSpaceShape
{
public:
   ScreenSpaceShape() {}
   ~ScreenSpaceShape() {}

};

class ScreenSpaceRect;

class ScreenSpaceNode : public Horde3D::SceneNode
{
public:
   static Horde3D::SceneNode* factoryFunc( const Horde3D::SceneNodeTpl &nodeTpl );
   static Horde3D::SceneNodeTpl* parsingFunc( std::map< std::string, std::string > &attribs );

   ~ScreenSpaceNode();

   ScreenSpaceRect* addRect(int left, int right, int top, int bottom, 
      csg::Color3& borderColor=csg::Color3(255,255,255),
      int borderWidth=2,
      bool filled=false,
      csg::Color3& fillColor=csg::Color3(255,255,255));

protected:
   void init();

   ScreenSpaceNode( const ScreenSpaceNodeTpl& screenSpaceTpl );


private:
   std::vector<ScreenSpaceShape*> shapes_;

   //friend class SceneManager;
   //friend class Renderer;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
