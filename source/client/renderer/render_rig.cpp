#include "pch.h"
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include "renderer.h"
#include "render_entity.h"
#include "render_rig.h"
#include "pipeline.h"
#include "om/components/render_rig.h"
#include "Horde3DUtils.h"
#include "resources/res_manager.h"
#include "qubicle_file.h"
#include "texture_color_mapper.h"
#include "pipeline.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderRig::RenderRig(RenderEntity& entity, om::RenderRigPtr rig) :
   entity_(entity),
   rig_(rig)
{
   ASSERT(rig);

   animationTableName_ = rig->GetAnimationTable();

   const auto& rigs = rig->GetRigs();

   auto added = std::bind(&RenderRig::AddRig, this, std::placeholders::_1);
   auto removed = std::bind(&RenderRig::RemoveRig, this, std::placeholders::_1);

   scale_ = rig->GetScale();
   entity_.GetSkeleton().SetScale(scale_);

   tracer_ += rigs.TraceSetChanges("render rig rigs", added, removed);
   UpdateRig(rigs);
}

RenderRig::~RenderRig()
{
   DestroyAllRenderNodes();
}

void RenderRig::UpdateRig(const dm::Set<std::string>& rigs)
{
   DestroyAllRenderNodes();
   for (const auto& entry : rigs) {
      AddRig(entry);
   }
}

void RenderRig::AddRig(const std::string& identifier)
{
   if (boost::algorithm::ends_with(identifier, ".qb")) {
      AddQubicleResource(identifier);
   } else {
      ASSERT(false);
   }
}

void RenderRig::AddQubicleResource(const std::string uri)
{
   QubicleFile f;
   std::vector<H3DNode>& nodes = nodes_[uri];
   DestroyRenderNodes(nodes);

   // xxx: no.  Make a qubicle resource type so they only get loaded once, ever.
   std::ifstream input;
   if (!resources::ResourceManager2::GetInstance().OpenResource(uri, input)) {
      return;
   }
   input >> f;

   std::shared_ptr<resources::DataResource> skeleton;
   if (!animationTableName_.empty()) {
      skeleton = resources::ResourceManager2::GetInstance().Lookup<resources::DataResource>(animationTableName_);
   }
   
   auto getBonePos = [&skeleton](std::string bone) -> csg::Point3f {
      csg::Point3f pos(0, 0, 0);
      if (skeleton) {
         JSONNode const& bones = skeleton->GetJson()["skeleton"];
         auto i = bones.find(bone);
         if (i != bones.end() && i->type() == JSON_ARRAY && i->size() == 3) {
            for (int j = 0; j < 3; j++) {
               pos[j] = (float)i->at(j).as_float();
            }
         }
      }
      return pos;
   };

   for (const auto& entry : f) {
      // Qubicle requires that ever matrix in the file have a unique name.  While authoring,
      // if you copy/pasta a matrix, it will rename it from matrixName to matrixName_2 to
      // dismabiguate them.  Ignore everything after the _ so we don't make authors manually
      // rename every single part when this happens.
      std::string bone = entry.first;
      int pos = bone.find('_');
      if (pos != std::string::npos) {
         bone = bone.substr(0, pos);
      }

      csg::Point3f origin = getBonePos(bone);
      auto& pipeline = Pipeline::GetInstance();
      Pipeline::Geometry geo = pipeline.OptimizeQubicle(entry.second, origin);

      auto& mapper = TextureColorMapper::GetInstance();
      // Geometry resource names must be unique.  WAT????
      Pipeline::GeometryResource gr = pipeline.CreateMesh(uri + "_" + bone, geo);

      H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone);
      H3DNode node = h3dAddModelNode(parent, "blocks", gr.geometry);
      H3DNode mesh = h3dAddMeshNode(node , "mesh", mapper.GetMaterial(), 0, gr.numIndices, 0, gr.numVertices - 1);

      h3dSetNodeTransform(node, 0, 0, 0, 0, 0, 0, scale_, scale_, scale_);
      nodes.push_back(node);
      // LOG(WARNING) << "Adding render rig node " << node;

      /*
      H3DNode node = entity_.GetSkeleton().AttachEntityToBone(res, bone, math3d::point3(0, 0, 0));
      //LOG(WARNING) << "  attaching " << model.GetMesh() << " to " << bone;
      h3dSetNodeTransform(node, 0, 0, 0, 0, 0, 0, .1f, .1f, .1f);
      nodes.push_back(node);
      */
   }
}

void RenderRig::RemoveRig(const std::string& identifier)
{
   DestroyRenderNodes(nodes_[identifier]);
   nodes_.erase(identifier);
}

void RenderRig::DestroyRenderNodes(std::vector<H3DNode>& nodes)
{
   for (auto n : nodes) {
      LOG(WARNING) << "Destroying render rig node " << n;
      h3dRemoveNode(n);
      //h3dSetNodeFlags(n, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true);
   }
   nodes.clear();
}

void RenderRig::DestroyAllRenderNodes()
{
   for (auto& entry : nodes_) {
      DestroyRenderNodes(entry.second);
   }
   nodes_.clear();
}
