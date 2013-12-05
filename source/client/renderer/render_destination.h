#ifndef _RADIANT_CLIENT_RENDER_DESTINATION_H
#define _RADIANT_CLIENT_RENDER_DESTINATION_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "csg/color.h"
#include "render_component.h"
#include "horde3d/extensions/stockpile/stockpile_node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderDestination : public RenderComponent {
   public:
      RenderDestination(const RenderEntity& entity, om::DestinationPtr stockpile);
      ~RenderDestination();

   private:
      void RenderDestinationRegion(int i, om::DeepRegionGuardPtr trace, csg::Color4 const& color);
      void RemoveDestinationRegion(int i);

   private:
      enum Regions {
         REGION = 0,
         ADJACENT,
         COUNT,
      };

   private:
      const RenderEntity*     entity_;
      om::DestinationPtr      destination_;
      H3DNodeUnique           regionDebugShape_[COUNT];
      om::DeepRegionGuardPtr  region_trace_[COUNT];
      core::Guard             renderer_guard_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_DESTINATION_H
