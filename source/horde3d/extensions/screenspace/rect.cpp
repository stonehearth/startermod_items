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

#include "rect.h"
#include "extension.h"
#include "radiant.h"

using namespace ::radiant;
using namespace ::radiant::horde3d;


ScreenSpaceRect::ScreenSpaceRect(H3DNode parent, int left, int right, int top, int bottom)
{
   float posData[] = {
      left, top, 0.0,
      right, top, 0.0,
      right, bottom, 0.0,
      left, bottom, 0.0
   };

   uint32 indexData[] = {
      0, 1, 2,
      0, 2, 3
   };

   H3DRes geo = h3dutCreateGeometryRes("foo", 4, 6, posData, indexData, nullptr, nullptr, nullptr, nullptr, nullptr);
   H3DRes matRes = h3dAddResource(H3DResTypes::Material, "materials/rect.material.xml", 0);
   H3DNode modelNode = h3dAddModelNode(parent, "rect_model", geo);
   h3dAddMeshNode(modelNode, "rect_mesh", matRes, 0, 6, 0, 3);
}

ScreenSpaceRect::~ScreenSpaceRect()
{

}
