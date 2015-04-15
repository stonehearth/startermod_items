#ifndef _RADIANT_CLIENT_RENDER_DESTINATION_H
#define _RADIANT_CLIENT_RENDER_DESTINATION_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "csg/color.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderDestination : public RenderComponent {
   public:
      RenderDestination(const RenderEntity& entity, om::DestinationPtr stockpile);
      ~RenderDestination();

   private:
      void RenderDestinationRegion(int i, om::DeepRegion3fGuardPtr trace, csg::Color4 const& color);
      void RemoveDestinationRegion(int i);

   private:
      enum Regions {
         REGION = 0,
         RESERVED,
         ADJACENT,
         COUNT,
      };

   private:
      RenderEntity const&      entity_;
      om::DestinationPtr       destination_;
      H3DNodeUnique            regionDebugShape_[COUNT];
      om::DeepRegion3fGuardPtr region_trace_[COUNT];
      core::Guard              renderer_guard_;
      core::Guard              _debugShapesEnabledGuard;
      bool                     _visible;
      bool                     _enabled;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_DESTINATION_H
