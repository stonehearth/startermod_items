#include "pch.h"
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include "renderer.h"
#include "render_entity.h"
#include "render_render_info.h"
#include "pipeline.h"
#include "om/components/model_variants.h"
#include "om/components/render_info.h"
#include "Horde3DUtils.h"
#include "resources/res_manager.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "pipeline.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderRenderInfo::QubicleFileMap RenderRenderInfo::qubicle_map__;

std::shared_ptr<voxel::QubicleFile> RenderRenderInfo::LoadQubicleFile(std::string const& uri)
{
   auto i = qubicle_map__.find(uri);
   if (i != qubicle_map__.end()) {
      return i->second;
   }
   std::ifstream input;
   res::ResourceManager2::GetInstance().OpenResource(uri, input);
   ASSERT(input.good());
   std::shared_ptr<voxel::QubicleFile> q = std::make_shared<voxel::QubicleFile>();
   input >> *q;

   qubicle_map__[uri] = q;
   return q;
}

void RenderRenderInfo::SetDirtyBits(int flags)
{
   dirty_ |= flags;

   if (renderer_frame_trace_.Empty()) {
      renderer_frame_trace_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         Update();
      });
      ASSERT(!renderer_frame_trace_.Empty());
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

   render_info_guards_ += render_info->GetScale().TraceObjectChanges("scale changed in render_info", set_scale_dirty_bit);

   // if the model variant changes...
   render_info_guards_ += render_info->GetModelVariant().TraceObjectChanges("model variant changed in render_info", set_model_dirty_bit);

   // if any entry in the attached items thing changes...
   render_info_guards_ += render_info->GetAttachedEntities().TraceObjectChanges("attached changed in render_info", set_model_dirty_bit);

   // if the material changes...
   render_info_guards_ += render_info->GetMaterial().TraceObjectChanges("material changed in render_info", set_model_dirty_bit);
   render_info_guards_ += render_info->GetModelMode().TraceObjectChanges("model_mode changed in render_info", set_model_dirty_bit);
   SetDirtyBits(-1);
}

RenderRenderInfo::~RenderRenderInfo()
{
}

void RenderRenderInfo::AccumulateModelVariant(ModelMap& m, om::ModelVariantPtr v)
{
   if (v) {
      om::ModelVariant::Layer layer = v->GetLayer();

      auto set_model_dirty_bit = [=]() {
         SetDirtyBits(MODEL_DIRTY);
      };
      variant_guards_ += v->GetModels().TraceObjectChanges("model variant changed in render_info", set_model_dirty_bit);

      for (std::string const& model : v->GetModels()) {
         std::shared_ptr<voxel::QubicleFile> qubicle;
         try {
            qubicle = LoadQubicleFile(model);
         } catch (res::Exception& e) {
            LOG(WARNING) << "could not load qubicle file: " << e.what();
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
   std::string const& material_path = *render_info->GetMaterial();
   if (material_path_ != material_path) {
      material_path_  = material_path;
      H3DRes material = h3dAddResource(H3DResTypes::Material, material_path.c_str(), 0);
      material_.reset(material);
   }
}

void RenderRenderInfo::RebuildModels(om::RenderInfoPtr render_info)
{
   ModelMap all_models;

   variant_guards_.Clear();
   std::string current_variant = GetModelVariant(render_info);
   for (om::EntityRef const& a : render_info->GetAttachedEntities()) {
      auto attached = a.lock();
      if (attached) {
         auto mv = attached->GetComponent<om::ModelVariants>();
         AccumulateModelVariants(all_models, mv, current_variant);
      }
   }
   auto mv = entity_.GetEntity()->GetComponent<om::ModelVariants>();
   AccumulateModelVariants(all_models, mv, current_variant);

   FlatModelMap flattened;
   FlattenModelMap(all_models, flattened);
   RemoveObsoleteNodes(flattened);
   AddMissingNodes(render_info, flattened);
}

void RenderRenderInfo::FlattenModelMap(ModelMap& m, FlatModelMap& flattened)
{
   for (const auto& entry : m) {
      voxel::QubicleMatrix const* matrix = nullptr;
      for (int i = om::ModelVariant::NUM_LAYERS - 1; i >= 0; i--) {
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

void RenderRenderInfo::AddModelNode(om::RenderInfoPtr render_info, std::string const& bone, voxel::QubicleMatrix const* matrix)
{
   ASSERT(nodes_.find(bone) == nodes_.end());

   csg::Point3f origin = csg::Point3f::zero;
   auto i = bones_offsets_.find(bone);
   if (i != bones_offsets_.end()) {
      origin = i->second;
   }
   float scale = *render_info->GetScale();

   auto& pipeline = Pipeline::GetInstance();
   H3DNode mesh;
   H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone);

   H3DNodeUnique node;
   if (model_mode_ == "opaque") {
      csg::Region3 model = voxel::QubicleBrush(matrix)
                                 .SetPaintMode(voxel::QubicleBrush::Opaque)
                                 .SetPreserveMatrixOrigin(true)
                                 .Paint();
      csg::mesh_tools::mesh geometry = csg::mesh_tools().ConvertRegionToMesh(model);
      node = pipeline.AddMeshNode(parent, geometry, &mesh);
   } else {
      node = pipeline.AddQubicleNode(parent, *matrix, origin, &mesh);
   }
   if (material_.get()) {
      h3dSetNodeParamI(mesh, H3DMesh::MatResI, material_.get());
   }
   h3dSetNodeTransform(node.get(), 0, 0, 0, 0, 0, 0, scale, scale, scale);
   nodes_[bone] = NodeMapEntry(matrix, node);
}

void RenderRenderInfo::AddMissingNodes(om::RenderInfoPtr render_info, FlatModelMap const& m)
{
   auto i = m.begin();
   while (i != m.end()) {
      auto j = nodes_.find(i->first);
      if (j == nodes_.end() || i->second != j->second.matrix) {
         AddModelNode(render_info, i->first, i->second);
      }
      i++;
   }
}

void RenderRenderInfo::RebuildBoneOffsets(om::RenderInfoPtr render_info)
{
   bones_offsets_.clear();
   std::string const& animation_table_name = *render_info->GetAnimationTable();
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
         //LOG(WARNING) << "updating render_info for " << entity_.GetEntity();
         if (dirty_ & ANIMATION_TABLE_DIRTY) {
            RebuildBoneOffsets(render_info);
            SetDirtyBits(MODEL_DIRTY);
         }
         if (dirty_ & MODEL_DIRTY) {
            model_mode_ = *render_info->GetModelMode();
            CheckMaterial(render_info);
            RebuildModels(render_info);
         }
         if (dirty_ & SCALE_DIRTY) {
            entity_.GetSkeleton().SetScale(*render_info->GetScale());
         }
      }
      dirty_ = 0;
   }
   renderer_frame_trace_.Clear();
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
   return use_model_variant_override_ ? model_variant_override_ : *render_info->GetModelVariant();
}
