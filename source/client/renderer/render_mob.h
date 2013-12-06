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
   void RenderAxes();
   void RemoveAxes();
   void Move();
   void UpdateTransform(csg::Transform const& transform);
   void UpdateInterpolate(bool interpolate);

private:
   const RenderEntity&  entity_;
   om::MobRef           mob_;
   core::Guard          show_debug_shape_guard_;
   core::Guard          renderer_guard_;
   csg::Transform      _initial;
   csg::Transform      _final;
   csg::Transform      _current;
   H3DNodeUnique       _axes;
   bool                 first_update_;
   dm::ObjectId         entity_id_;
   dm::TracePtr         interp_trace_;
   dm::TracePtr         transform_trace_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_MOB_H
