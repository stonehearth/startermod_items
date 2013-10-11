#ifndef _RADIANT_CLIENT_RENDER_MOB_H
#define _RADIANT_CLIENT_RENDER_MOB_H

#include <map>
#include "namespace.h"
#include "render_component.h"
#include "om/om.h"
#include "dm/dm.h"
#include "csg/transform.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderMob : public RenderComponent {
public:
   RenderMob(const RenderEntity& entity, om::MobPtr mob);

private:
   void Move();
   void Update();
   void Interpolate();
   void StartInterpolate();

private:
   const RenderEntity&  entity_;
   om::MobRef           mob_;
   dm::Guard              tracer_;
   csg::Transform    _initial;
   csg::Transform    _final;
   csg::Transform    _current;
   H3DNodeUnique     _axes;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_MOB_H
