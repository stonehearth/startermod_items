#include "pch.h"
#include "render_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

static void UpdateShape(csg::Region3 const& region, H3DNode shape, csg::Color4 const& color)
{
}

void client::CreateRegionDebugShape(H3DNode parent,
                                    H3DNodeUnique& shape,
                                    om::DeepRegionGuardPtr trace,
                                    csg::Color4 const& color)
{

   static std::atomic<int> count = 1;
   std::string name = "debug region " + stdutil::ToString(count++);

   H3DNode s = h3dRadiantAddDebugShapes(parent, name.c_str());
   shape = H3DNodeUnique(s);

   trace->OnChanged([s, color](csg::Region3 const& r) {
         h3dRadiantClearDebugShape(s);
         h3dRadiantAddDebugRegion(s, r, color);
         h3dRadiantCommitDebugShape(s);
      })
      ->PushObjectState();
}
