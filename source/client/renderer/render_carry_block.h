#ifndef _RADIANT_CLIENT_RENDER_CARRY_BLOCK_H
#define _RADIANT_CLIENT_RENDER_CARRY_BLOCK_H

#include <unordered_map>
#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "dm/dm.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderCarryBlock : public RenderComponent {
public:
   RenderCarryBlock(RenderEntity& entity, om::CarryBlockPtr renderGrid);
   ~RenderCarryBlock();

private:
   void UpdateCarrying();

private:
   RenderEntity&        entity_;
   core::Guard            tracer_;
   om::CarryBlockRef    carryBlock_;
   dm::ObjectId         carrying_;
   H3DNode              carryBone_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_CARRY_BLOCK_H
