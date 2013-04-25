#ifndef _RADIANT_CLIENT_RENDERER_RENDER_ASPECT_H
#define _RADIANT_CLIENT_RENDERER_RENDER_ASPECT_H

#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderAspect {
   public:
      virtual ~RenderAspect() { }
      virtual void PrepareToRender(int now, float distance) { }
      virtual void AddToSelection(om::Selection& sel, const math3d::ray3& ray, const math3d::point3& intersection, const math3d::point3& normal) { }
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDERER_RENDER_ASPECT_H
 