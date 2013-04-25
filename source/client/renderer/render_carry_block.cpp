#include "pch.h"
#include "renderer.h"
#include "render_carry_block.h"
#include "om/components/carry_block.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderCarryBlock::RenderCarryBlock(RenderEntity& entity, om::CarryBlockPtr carryBlock) :
   entity_(entity),
   carryBlock_(carryBlock),
   carrying_(0)
{
   carryBoneId_ = entity_.GetSkeleton().GetSceneNode("carry");

   tracer_ += carryBlock->TraceCarrying("render carry block", std::bind(&RenderCarryBlock::UpdateCarrying, this));
   UpdateCarrying();
}

RenderCarryBlock::~RenderCarryBlock()
{
}

void RenderCarryBlock::UpdateCarrying()
{
   LOG(WARNING) << "updating carry block.";
   auto carryBlock = carryBlock_.lock();
   if (carryBlock && carryBoneId_) {
      om::EntityId carryingId = 0;
      om::EntityPtr carrying = carryBlock->GetCarrying().lock();      
      if (carrying) {
         carryingId = carrying->GetEntityId();
      }
      if (carryingId != carrying_) {
         // If the thing we used to be carrying is still attached to our carry bone, deparent it
         if (carrying_) {
            auto renderObject = Renderer::GetInstance().GetRenderObject(2, carrying_); // xxx hard coded client store id =..(
            if (renderObject && renderObject->GetParent() == carryBoneId_) {
               LOG(WARNING) << "setting render object " << carrying_ << " parent to " << 0;
               renderObject->SetParent(0);
            } else {
               LOG(WARNING) << "not reparenting old carrying.  already reparented off carry bone";
            }
         }

         carrying_ = carryingId;

         if (carrying_) {            
            // Update carrying and put it on the carry bone
            auto renderObject = Renderer::GetInstance().GetRenderObject(2, carrying_); // xxx hard coded client store id =..(
            if (renderObject ) {
               // If this render object doesn't exist yet, we should go ahead and create it (??)
               LOG(WARNING) << "setting render object " << carrying_ << " parent to " << carryBoneId_;
               renderObject->SetParent(carryBoneId_);
            }
         }
      }
   }
}
