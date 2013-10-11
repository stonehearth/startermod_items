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
      H3DNodeUnique              regionDebugShape_;
      H3DNodeUnique              reservedDebugShape_;
      H3DNodeUnique              adjacentDebugShape_;
      om::DeepRegionGuardPtr     region_guard_;
      om::DeepRegionGuardPtr     reserved_guard_;
      om::DeepRegionGuardPtr     adjacent_guard_;

};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_DESTINATION_H
