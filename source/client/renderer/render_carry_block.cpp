#include "pch.h"
#include "renderer.h"
#include "render_carry_block.h"
#include "client/client.h"
#include "om/entity.h"
#include "om/components/carry_block.ridl.h"
#include "dm/record_trace.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define CB_LOG(level)      LOG(renderer.carry_block, level)

RenderCarryBlock::RenderCarryBlock(RenderEntity& entity, om::CarryBlockPtr carryBlock) :
   entity_(entity),
   carryBlock_(carryBlock),
   carrying_(0)
{
   carryBone_ = entity_.GetSkeleton().GetSceneNode("carry");

   trace_ = carryBlock->TraceChanges("render", dm::RENDER_TRACES)
                           ->OnModified([this]() {
                                 UpdateCarrying();
                           });
   
   UpdateCarrying();
}

RenderCarryBlock::~RenderCarryBlock()
{
}

void RenderCarryBlock::UpdateCarrying()
{
   CB_LOG(5) << "updating carry block.";
   auto carryBlock = carryBlock_.lock();
   if (carryBlock && carryBone_) {
      dm::ObjectId carryingId = 0;
      om::EntityPtr carrying = carryBlock->GetCarrying().lock();      
      if (carrying) {
         carryingId = carrying->GetObjectId();
      }
      if (carryingId != carrying_) {
         // If the thing we used to be carrying is still attached to our carry bone, deparent it
         if (carrying_) {
            auto renderObject = Renderer::GetInstance().GetRenderObject(2, carrying_); // xxx hard coded client store id =..(
            if (renderObject && renderObject->GetParent() == carryBone_) {
               CB_LOG(5) << "setting render object " << carrying_ << " parent to " << 0;
               renderObject->SetParent(0);
            } else {
               CB_LOG(5) << "not reparenting old carrying.  already reparented off carry bone";
            }
         }

         carrying_ = carryingId;

         if (carrying_) {            
            // Update carrying and put it on the carry bone
            // xxx: we really really don't want to have to grab the raw entity, but this might be
            // the first time we've ever seen it and the render object may not exist yet!  i think
            // this can be fixed by verifying all alloc' callbacks have fired (and all render objects
            // created) before firing traces, but who knows...
            om::EntityPtr entity = Client::GetInstance().GetStore().FetchObject<om::Entity>(carrying_);
            if (entity) {
               CB_LOG(5) << "setting render object " << carrying_ << " parent to " << carryBone_;
               Renderer::GetInstance().CreateRenderObject(carryBone_, entity);
            }
         }
      }
   }
}
