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
      void UpdateShape(csg::Region3 const& region, H3DNode shape, csg::Color4 const& color);
      void CreateDebugTracker(om::BoxedRegionGuardPtr& guard, std::string const& regionName, H3DNodeUnique& shape, dm::Boxed<om::BoxedRegion3Ptr> const& region, csg::Color4 const& color);

   private:
      const RenderEntity&        entity_;
      H3DNodeUnique              regionDebugShape_;
      H3DNodeUnique              reservedDebugShape_;
      H3DNodeUnique              adjacentDebugShape_;
      H3DNodeUnique              node_;
      dm::Guard                  guards_;
      om::BoxedRegionGuardPtr    region_guard_;
      om::BoxedRegionGuardPtr    reserved_guard_;
      om::BoxedRegionGuardPtr    adjacent_guard_;

};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_DESTINATION_H
