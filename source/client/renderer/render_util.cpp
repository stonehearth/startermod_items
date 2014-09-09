#include "pch.h"
#include "render_util.h"
#include "physics/physics_util.h"

using namespace ::radiant;
using namespace ::radiant::client;

static void UpdateShape(csg::Region3 const& region, H3DNode shape, csg::Color4 const& color)
{
}

template <typename BoxedRegion>
void client::CreateRegionDebugShape(om::EntityRef entityRef,
                                    H3DNodeUnique& shape,
                                    std::shared_ptr<om::DeepRegionGuard<dm::Boxed<std::shared_ptr<BoxedRegion>>>> trace,
                                    csg::Color4 const& color)
{
   // offset of the local coordinates from their terrain aligned world coordiantes
   static const csg::Point3f offset(0.5f, 0, 0.5f);
   static std::atomic<int> count = 1;
   std::string name = "debug region " + stdutil::ToString(count++);

   // add under the root node since we're providing world coordinates
   H3DNode s = h3dRadiantAddDebugShapes(1, name.c_str());
   shape = H3DNodeUnique(s);

   trace->OnChanged([s, color, entityRef](BoxedRegion::Value const& localRegion) {
         om::EntityPtr entity = entityRef.lock();
         if (entity) {
            BoxedRegion::Value worldRegion = phys::LocalToWorld(localRegion, entity);
            h3dRadiantClearDebugShape(s);
            // h3dRadiantAddDebugRegion will subtract the offset from the region coordinates
            h3dRadiantAddDebugRegion(s, csg::ToFloat(worldRegion), offset, color);
            h3dRadiantCommitDebugShape(s);
         }
      })
      ->PushObjectState();
}

template void client::CreateRegionDebugShape(om::EntityRef entityRef,
                                             H3DNodeUnique& shape,
                                             om::DeepRegion3GuardPtr trace,
                                             csg::Color4 const& color);


template void client::CreateRegionDebugShape(om::EntityRef entityRef,
                                             H3DNodeUnique& shape,
                                             om::DeepRegion3fGuardPtr trace,
                                             csg::Color4 const& color);
