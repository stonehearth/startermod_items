#ifndef _RADIANT_CLIENT_RENDER_RENDER_REGION_H
#define _RADIANT_CLIENT_RENDER_RENDER_REGION_H

#include <map>
#include "namespace.h"
#include "render_component.h"
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
   void OnSelected(om::Selection& sel, const csg::Ray3& ray,
                   const csg::Point3f& intersection, const csg::Point3f& normal);

private:
   const RenderEntity&        entity_;
   om::DeepRegionGuardPtr    region_guard_;
   core::Guard                  selectedGuard_;
   H3DNodeUnique              node_;
   std::vector<H3DNodeUnique> meshes_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_RENDER_REGION_H
