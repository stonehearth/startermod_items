#ifndef _RADIANT_CLIENT_RENDER_PAPERDOLL_H
#define _RADIANT_CLIENT_RENDER_PAPERDOLL_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "dm/set.h"
#include "types.h"
#include "skeleton.h"
#include "render_entity.h"
#include "render_component.h"
#include "resources/animation.h"
#include "om/components/paperdoll.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderPaperdoll : public RenderComponent {
public:
   RenderPaperdoll(RenderEntity& entity, om::PaperdollPtr rig);
   ~RenderPaperdoll();

private:
   void UpdateSlot(int i, om::EntityRef obj);

private:
   static const char* nodes_[om::Paperdoll::NUM_SLOTS];

private:
   RenderEntity&        entity_;
   om::PaperdollRef     paperdoll_;
   dm::GuardSet         tracer_;
   RenderEntityPtr      slots_[om::Paperdoll::NUM_SLOTS];
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_PAPERDOLL_H
