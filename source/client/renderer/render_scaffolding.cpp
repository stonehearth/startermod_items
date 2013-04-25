#include "pch.h"
#include "renderer.h"
#include "render_entity.h"
#include "render_scaffolding.h"
#include "om/components/scaffolding.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderScaffolding::RenderScaffolding(const RenderEntity& entity, om::ScaffoldingPtr scaffolding) :
   RenderBuildOrder(entity, scaffolding),
   scaffolding_(scaffolding),
   blocksNode_(0)
{
   normal_ = scaffolding->GetNormal();

   H3DNode parent = entity.GetNode();
   std::string name = h3dGetNodeParamStr(parent, H3DNodeParams::NameStr) + std::string(" scaffolding build order");

   ladderDebugShape_ = h3dRadiantAddDebugShapes(parent, name.c_str());
   traces_ += scaffolding->TraceLadderRegion("render build_order region", [=](const csg::Region3& region) {
      UpdateLadder(region);
   });
   UpdateLadder(scaffolding->GetLadderRegion());

   if (normal_[0] == 1 && normal_[2] == 0) {
      rotation_ = 270.0f;
   } else if (normal_[0] == 0 && normal_[2] == 1) {
      rotation_ = 180.0f;
   } else if (normal_[0] == -1 && normal_[2] == 0) {
      rotation_ = 90.0f;
   } else {
      rotation_ = 0.0f;
   }
}

RenderScaffolding::~RenderScaffolding()
{
   if (blocksNode_) {
      h3dRemoveNode(blocksNode_);
   }
}

void RenderScaffolding::UpdateRegion(const csg::Region3& region)
{
   RenderBuildOrder::UpdateRegion(region);
   auto scaffolding = GetScaffolding();
   if (scaffolding) {

      // xxx -- Add incremental updating
      if (blocksNode_) {
         h3dRemoveNode(blocksNode_);
      }

      ostringstream name;
      name << "Scaffolding " << scaffolding->GetObjectId();
      blocksNode_ = h3dAddGroupNode(GetParentNode(), name.str().c_str());

      if (!region.IsEmpty()) {
         csg::Point3 normal = scaffolding->GetNormal();
         int tangent = normal_.x ? 2 : 0;
         int top = region.GetBounds().GetMax()[1] - 1;
         for (const auto& r : region) {
            for (const auto& p : r) {
               bool isTop = !region.Contains(p + csg::Point3(0, 1, 0));
               bool isInner = region.Contains(p + normal);

               if (isInner && !isTop) {
                  // xxx: should really just iterate over the outer slice and 
                  // the top slice.
                  continue;
               }
               ostringstream segment;
               if (isTop) {
                  if (isInner) {
                     segment << "models/scaffolding/scaffold_top.scene.xml";
                  } else {
                     segment << "models/scaffolding/scaffold_top_" << (p[tangent] % 4) << ".scene.xml";
                  }
               } else {
                  segment << "models/scaffolding/scaffold_" << (p[tangent] % 4) << "_" << (p[1] % 4) << ".scene.xml";
               }

               H3DNode res = h3dAddResource(H3DResTypes::SceneGraph, segment.str().c_str(), 0);
               ASSERT(res);

               Renderer::GetInstance().LoadResources();

               H3DNode node = h3dAddNodes(blocksNode_, res);
               ASSERT(node);

               //math3d::ipoint3 pt = math3d::ipoint3(p) + math3d::ipoint3(normal_);
               h3dSetNodeTransform(node, (float)p.x, (float)p.y, (float)p.z, 0, rotation_, 0, .1f, .1f, .1f);
               /*
               if (isTop) {
                  res = h3dAddResource(H3DResTypes::SceneGraph, "models/scaffolding/scaffold_top.scene.xml", 0);

                  Renderer::GetInstance().LoadResources();

                  node = h3dAddNodes(blocksNode_, res);

                  ASSERT(res && node);

                  pt = math3d::ipoint3(p);
                  h3dSetNodeTransform(node, (float)pt.x, (float)pt.y, (float)pt.z, 0, rotation_, 0, .1f, .1f, .1f);
               }
               */
            }
         }
      }
   }
}

void RenderScaffolding::UpdateLadder(const csg::Region3& region)
{
   if (entity_.ShowDebugRegions()) {
      h3dRadiantClearDebugShape(ladderDebugShape_);
      h3dRadiantAddDebugRegion(ladderDebugShape_, region, math3d::color4(0, 0, 255, 192));
      h3dRadiantCommitDebugShape(ladderDebugShape_);
   }
}

