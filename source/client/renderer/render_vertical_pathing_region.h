#ifndef _RADIANT_CLIENT_RENDER_VERTICAL_PATHING_REGION_H
#define _RADIANT_CLIENT_RENDER_VERTICAL_PATHING_REGION_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "csg/color.h"
#include "render_component.h"
#include "horde3d/extensions/stockpile/stockpile_node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderVerticalPathingRegion : public RenderComponent {
   public:
      RenderVerticalPathingRegion(const RenderEntity& entity, om::VerticalPathingRegionPtr stockpile);
      ~RenderVerticalPathingRegion();

   private:
      H3DNodeUnique             regionDebugShape_;
      om::DeepRegionGuardPtr    region_guard_;

};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_VERTICAL_PATHING_REGION_H
