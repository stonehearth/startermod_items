#ifndef _RADIANT_CLIENT_RENDER_BUILD_ORDER_H
#define _RADIANT_CLIENT_RENDER_BUILD_ORDER_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "types.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderBuildOrder : public RenderComponent {
   public:
      RenderBuildOrder(const RenderEntity& entity, om::BuildOrderPtr buildOrder);
      ~RenderBuildOrder();

   protected:
      virtual void UpdateRegion(const csg::Region3& region);
      virtual void UpdateReserved(const csg::Region3& region);
      H3DNode GetParentNode() const;

   protected:
      const RenderEntity&     entity_;
      H3DNode                 regionDebugShape_;
      H3DNode                 reservedDebugShape_;
      dm::GuardSet            traces_;

};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_BUILD_ORDER_H
