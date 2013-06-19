#include "pch.h"
#include "radiant_json.h"
#include "renderer.h"
#include "render_entity.h"
#include "render_scaffolding.h"
#include "om/entity.h"
#include "om/components/scaffolding.h"
#include "resources/res_manager.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "pipeline.h"

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
      rotation_ = 90.0f;
   } else if (normal_[0] == 0 && normal_[2] == 1) {
      rotation_ = 0.0f;
   } else if (normal_[0] == -1 && normal_[2] == 0) {
      rotation_ = 270.0f;
   } else {
      rotation_ = 180.0f;
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

      std::ostringstream name;
      name << "Scaffolding " << scaffolding->GetObjectId();
      blocksNode_ = h3dAddGroupNode(GetParentNode(), name.str().c_str());

      if (region.IsEmpty()) {
         return;
      }
      csg::Point3 normal = scaffolding->GetNormal();
      int tangent = normal_.x ? 2 : 0;
      int top = region.GetBounds().GetMax()[1] - 1;

      om::EntityPtr e = entity_.GetEntity();
      if (!e) {
         return;
      }
      std::string uri = e->GetResourceUri();

      JSONNode const& res = resources::ResourceManager2::GetInstance().LookupJson(uri);
      JSONNode data = json::get<JSONNode>(res, "data");
      JSONNode models = json::get<JSONNode>(data, "models");
      

      for (const auto& r : region) {
         for (const auto& p : r) {
            bool isTop = !region.Contains(p + csg::Point3(0, 1, 0));
            bool isInner = region.Contains(p + normal);

            if (isInner && !isTop) {
               // xxx: should really just iterate over the outer slice and 
               // the top slice.
               continue;
            }
            std::ostringstream segment;
            segment << uri << "/";
            if (isTop) {
               if (isInner) {
                  segment << "plank.qb";
               } else {
                  segment << "top_" << (p[tangent] % 4) << ".qb";
               }
            } else {
               segment << "row" << (p[1] % 4) << "_" << (p[tangent] % 4) << ".qb";
            }
            LOG(WARNING) << segment.str();

            auto nodes = Pipeline::GetInstance().LoadQubicleFile(segment.str());
            if (!nodes.empty()) {
               H3DNode node = nodes.begin()->second;
               h3dSetNodeParent(node, blocksNode_);
               h3dSetNodeTransform(node, (float)p.x, (float)p.y, (float)p.z, 0, rotation_, 0, .1f, .1f, .1f);
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

