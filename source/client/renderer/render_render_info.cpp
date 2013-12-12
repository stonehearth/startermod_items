#include "pch.h"
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include "renderer.h"
#include "render_entity.h"
#include "render_render_info.h"
#include "pipeline.h"
#include "dm/set_trace.h"
#include "om/components/model_variants.ridl.h"
#include "om/components/render_info.ridl.h"
#include "Horde3DUtils.h"
#include "resources/res_manager.h"
#include "lib/perfmon/perfmon.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "pipeline.h"
#include "Horde3D.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define RI_LOG(level)      LOG(renderer.render_info, level)

void RenderRenderInfo::SetDirtyBits(int flags)
{
   dirty_ |= flags;

   if (renderer_frame_guard_.Empty()) {
      renderer_frame_guard_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         perfmon::TimelineCounterGuard tcg("update render_info");
         Update();
      });
   }
}

RenderRenderInfo::RenderRenderInfo(RenderEntity& entity, om::RenderInfoPtr render_info) :
   entity_(entity),
   render_info_(render_info),
   dirty_(-1),
   material_(0),
   use_model_variant_override_(false)
{
   auto set_scale_dirty_bit = [=]() {
      SetDirtyBits(SCALE_DIRTY);
   };
   auto set_model_dirty_bit = [=]() {
      SetDirtyBits(MODEL_DIRTY);
   };

   scale_trace_ = render_info->TraceScale("render", dm::RENDER_TRACES)
                                 ->OnModified(set_scale_dirty_bit);

   // if the variant we want to render changes...
   model_variant_trace_ = render_info->TraceModelVariant("render", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);

   // if any entry in the attached items thing changes...
   attached_trace_ = render_info->TraceAttachedEntities("render", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);

   // if the material changes...
   material_trace_ = render_info->TraceMaterial("render", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);
   mode_trace_ = render_info->TraceModelMode("render", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);

   SetDirtyBits(-1);
}

RenderRenderInfo::~RenderRenderInfo()
{
}

void RenderRenderInfo::AccumulateModelVariant(ModelMap& m, om::ModelLayerPtr v)
{
   if (v) {
      om::ModelLayer::Layer layer = v->GetLayer();

      variant_trace_ = v->TraceModels("render", dm::RENDER_TRACES)
                              ->OnModified([this]() {
                                 SetDirtyBits(MODEL_DIRTY);
                              });

      for (std::string const& model : v->EachModel()) {
         voxel::QubicleFile* qubicle;
         try {
            qubicle = Pipeline::GetInstance().LoadQubicleFile(model);
         } catch (res::Exception& e) {
            RI_LOG(1) << "could not load qubicle file: " << e.what();
            return;
         }
         for (const auto& entry : *qubicle) {
            std::string bone = GetBoneName(entry.first);
            voxel::QubicleMatrix const* matrix = &entry.second;

            m[bone].layers[layer] = matrix;
         }
      }
   }
}

void RenderRenderInfo::AccumulateModelVariants(ModelMap& m, om::ModelVariantsPtr model_variants, std::string const& current_variant)
{
   if (model_variants) {
      auto variant = model_variants->GetModelVariant(current_variant);
      if (variant) {
         AccumulateModelVariant(m, variant);
      }
   }
}

void RenderRenderInfo::CheckMaterial(om::RenderInfoPtr render_info)
{
   std::string const& material_path = render_info->GetMaterial();
   if (material_path_ != material_path) {
      material_path_  = material_path;
      H3DRes material = h3dAddResource(H3DResTypes::Material, material_path.c_str(), 0);
      material_.reset(material);
   }
}

void RenderRenderInfo::RebuildModels(om::RenderInfoPtr render_info)
{
   om::EntityPtr entity = entity_.GetEntity();

   if (entity) {
      ModelMap all_models;

      variant_trace_ = nullptr;
      std::string current_variant = GetModelVariant(render_info);
      for (om::EntityRef const& a : render_info->EachAttachedEntity()) {
         auto attached = a.lock();
         if (attached) {
            auto mv = attached->GetComponent<om::ModelVariants>();
            AccumulateModelVariants(all_models, mv, current_variant);
         }
      }
      auto mv = entity->GetComponent<om::ModelVariants>();
      AccumulateModelVariants(all_models, mv, current_variant);

      FlatModelMap flattened;
      FlattenModelMap(all_models, flattened);
      RemoveObsoleteNodes(flattened);
      AddMissingNodes(render_info, flattened);
   }
}

void RenderRenderInfo::FlattenModelMap(ModelMap& m, FlatModelMap& flattened)
{
   for (const auto& entry : m) {
      voxel::QubicleMatrix const* matrix = nullptr;
      for (int i = om::ModelLayer::NUM_LAYERS - 1; i >= 0; i--) {
         matrix = entry.second.layers[i];
         if (matrix != nullptr) {
            break;
         }
      }
      flattened[entry.first] = matrix;
   }
}


void RenderRenderInfo::RemoveObsoleteNodes(FlatModelMap const& m)
{
   auto i = nodes_.begin();
   while (i != nodes_.end()) {
      auto j = m.find(i->first);
      if (j == m.end() || j->second != i->second.matrix) {
         i = nodes_.erase(i);
      } else {
         i++;
      }
   }
}

std::string RenderRenderInfo::GetBoneName(std::string const& matrix_name)
{
   // Qubicle requires that ever matrix in the file have a unique name.  While authoring,
   // if you copy/pasta a matrix, it will rename it from matrixName to matrixName_2 to
   // dismabiguate them.  Ignore everything after the _ so we don't make authors manually
   // rename every single part when this happens.
   std::string bone = matrix_name;
   int pos = bone.find('_');
   if (pos != std::string::npos) {
      bone = bone.substr(0, pos);
   }
   return bone;
}

void RenderRenderInfo::AddModelNode(om::RenderInfoPtr render_info, std::string const& bone, voxel::QubicleMatrix const* matrix, float offset)
{
   ASSERT(nodes_.find(bone) == nodes_.end());

   csg::Point3f origin = csg::Point3f::zero;
   auto i = bones_offsets_.find(bone);
   if (i != bones_offsets_.end()) {
      origin = i->second;
   }
   float scale = render_info->GetScale();

   auto& pipeline = Pipeline::GetInstance();
   H3DNode mesh;
   H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone);

   H3DNodeUnique node;
   if (model_mode_ == "blueprint") {
      csg::Region3 model = voxel::QubicleBrush(matrix)
                                 .SetPaintMode(voxel::QubicleBrush::Opaque)
                                 .SetPreserveMatrixOrigin(true)
                                 .PaintOnce();
      node = pipeline.CreateBlueprintNode(parent, model, 0.5f, material_path_);
   } else {
      node = pipeline.AddQubicleNode(parent, *matrix, origin, &mesh);
      if (material_.get()) {
         h3dSetNodeParamI(mesh, H3DMesh::MatResI, material_.get());
      }
   }
   h3dSetNodeParamI(node.get(), H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(node.get(), H3DModel::PolygonOffsetF, 0, offset * 0.04f);
   h3dSetNodeParamF(node.get(), H3DModel::PolygonOffsetF, 1, offset * 10.0f);
   h3dSetNodeTransform(node.get(), 0, 0, 0, 0, 0, 0, scale, scale, scale);
   nodes_[bone] = NodeMapEntry(matrix, node);
}

void RenderRenderInfo::AddMissingNodes(om::RenderInfoPtr render_info, FlatModelMap const& m)
{
   float offset = 0;
   auto i = m.begin();
   while (i != m.end()) {
      auto j = nodes_.find(i->first);
      if (j == nodes_.end() || i->second != j->second.matrix) {
         AddModelNode(render_info, i->first, i->second, offset);
         offset += 1.0f;
      }
      i++;
   }
}

void RenderRenderInfo::RebuildBoneOffsets(om::RenderInfoPtr render_info)
{
   bones_offsets_.clear();
   std::string const& animation_table_name = render_info->GetAnimationTable();
   if (!animation_table_name.empty()) {
      json::Node table = res::ResourceManager2::GetInstance().LookupJson(animation_table_name);
      for (const auto& entry : table.get("skeleton", JSONNode())) {
         csg::Point3f pt;
         for (int j = 0; j < 3; j++) {
            pt[j] = json::Node(entry).get(j, 0.0f);
         }
         bones_offsets_[entry.name()] = pt;
      }
   }
}

void RenderRenderInfo::Update()
{
   if (dirty_) {
      auto render_info = render_info_.lock();
      if (render_info) {
         RI_LOG(5) << "updating render_info for " << entity_.GetObjectId();
         if (dirty_ & ANIMATION_TABLE_DIRTY) {
            RebuildBoneOffsets(render_info);
            SetDirtyBits(MODEL_DIRTY);
         }
         if (dirty_ & MODEL_DIRTY) {
            model_mode_ = render_info->GetModelMode();
            CheckMaterial(render_info);
            RebuildModels(render_info);
         }
         if (dirty_ & SCALE_DIRTY) {
            entity_.GetSkeleton().SetScale(render_info->GetScale());
         }
      }
      dirty_ = 0;
   }
   renderer_frame_guard_.Clear();
}

void RenderRenderInfo::SetModelVariantOverride(bool enabled, std::string const& variant)
{
   if (use_model_variant_override_ != enabled || (enabled && variant != model_variant_override_)) {
      SetDirtyBits(MODEL_DIRTY);
      model_variant_override_ = variant;
      use_model_variant_override_ = enabled;
   }
}

std::string RenderRenderInfo::GetModelVariant(om::RenderInfoPtr render_info) const
{
   ASSERT(render_info);
   return use_model_variant_override_ ? model_variant_override_ : render_info->GetModelVariant();
}
