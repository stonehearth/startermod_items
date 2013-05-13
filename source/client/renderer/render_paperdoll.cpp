#include "pch.h"
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include "pch.h"
#include "renderer.h"
#include "render_entity.h"
#include "render_paperdoll.h"
#include "pipeline.h"
#include "om/components/render_rig.h"
#include "Horde3DUtils.h"
#include "resources/res_manager.h"
#include "qubicle_file.h"
#include "texture_color_mapper.h"

using namespace ::radiant;
using namespace ::radiant::client;

const char* RenderPaperdoll::nodes_[om::Paperdoll::NUM_SLOTS] = {
   "mainHand",
   "",
};

RenderPaperdoll::RenderPaperdoll(RenderEntity& entity, om::PaperdollPtr paperdoll) :
   entity_(entity),
   paperdoll_(paperdoll)
{
   tracer_ += paperdoll->TraceSlots("render paperdoll", std::bind(&RenderPaperdoll::UpdateSlot, this, std::placeholders::_1, std::placeholders::_2));
   for (int i = 0; i < om::Paperdoll::NUM_SLOTS; i++) {
      UpdateSlot(i, paperdoll->GetItemInSlot((om::Paperdoll::Slot)i));
   }
}

RenderPaperdoll::~RenderPaperdoll()
{
}

void RenderPaperdoll::UpdateSlot(int i, om::EntityRef e)
{
   auto entity = e.lock();
   H3DNode parent =  entity_.GetSkeleton().GetSceneNode(nodes_[i]);

   if (parent) {
      if (entity) {
         auto renderObject = Renderer::GetInstance().CreateRenderObject(parent, entity);
         slots_[i] = renderObject;
      } else {
         if (slots_[i] && slots_[i]->GetParent() == parent) {
            slots_[i]->SetParent(0);
         }
         slots_[i] = nullptr;
      }
   }
}
