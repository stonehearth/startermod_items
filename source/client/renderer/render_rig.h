#ifndef _RADIANT_CLIENT_RENDER_RIG_H
#define _RADIANT_CLIENT_RENDER_RIG_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "dm/set.h"
#include "types.h"
#include "skeleton.h"
#include "render_component.h"
#include "resources/animation.h"
#include "om/components/render_rig.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderRig : public RenderComponent {
public:
   RenderRig(RenderEntity& entity, om::RenderRigPtr rig);
   ~RenderRig();

private:
   void UpdateRig(const dm::Set<std::string>& rigs);
   void AddRig(const std::string& rig_identifier);
   void RemoveRig(const std::string& identifier);
   void DestroyRenderNodes(std::vector<H3DNode>& nodes);
   void DestroyAllRenderNodes();

private:
   void AddQubicleResource(std::string identifier);

private:
   typedef std::unordered_map<std::string, std::vector<H3DNode>> RigMap;

   RenderEntity&        entity_;
   om::RenderRigRef     rig_;
   dm::Guard            tracer_;
   RigMap               nodes_;
   float                scale_;
   std::string          animationTableName_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_RIG_H
