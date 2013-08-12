#ifndef _RADIANT_CLIENT_RENDER_RENDER_REGION_H
#define _RADIANT_CLIENT_RENDER_RENDER_REGION_H

#include <map>
#include "namespace.h"
#include "render_component.h"
#include "types.h"
#include "om/om.h"
#include "dm/dm.h"
#include "csg/util.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderRenderRegion : public RenderComponent {
public:
   RenderRenderRegion(const RenderEntity& entity, om::RenderRegionPtr render_region);

private:
   void UpdateRenderRegion(csg::Region3 const& region);
   void OnSelected(om::Selection& sel, const math3d::ray3& ray,
                   const math3d::point3& intersection, const math3d::point3& normal);

private:
   const RenderEntity&  entity_;
   dm::Guard            regionGuard_;
   dm::Guard            selectedGuard_;
   H3DNode              node_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_RENDER_REGION_H
