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
      virtual void AddToSelection(om::Selection& sel, const csg::Ray3& ray, const csg::Point3f& intersection, const csg::Point3f& normal) { }
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDERER_RENDER_ASPECT_H
 