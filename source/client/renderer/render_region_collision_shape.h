#ifndef _RADIANT_CLIENT_RENDER_REGION_COLLISION_SHAPE_H
#define _RADIANT_CLIENT_RENDER_REGION_COLLISION_SHAPE_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "csg/color.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderRegionCollisionShape : public RenderComponent {
   public:
      RenderRegionCollisionShape(const RenderEntity& entity, om::RegionCollisionShapePtr stockpile);

   private:
      RenderEntity const&           entity_;
      om::RegionCollisionShapeRef   collision_shape_;
      om::DeepRegionGuardPtr        trace_;
      H3DNodeUnique                 shape_;
      core::Guard                   renderer_guard_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_REGION_COLLISION_SHAPE_H
