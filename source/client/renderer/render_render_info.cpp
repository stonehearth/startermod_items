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
#include "csg/region_tools.h"
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

   SetDirtyBits(-1);
}

RenderRenderInfo::~RenderRenderInfo()
{
}

void RenderRenderInfo::AccumulateModelVariant(ModelMap& m, om::ModelLayerPtr layer)
{
   if (layer) {
      om::ModelLayer::Layer level = layer->GetLayer();

      variant_trace_ = layer->TraceModels("render", dm::RENDER_TRACES)
                              ->OnModified([this]() {
                                 SetDirtyBits(MODEL_DIRTY);
                              });

      for (std::string const& model : layer->EachModel()) {
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

            m[bone].layers[level].push_back(matrix);
         }
      }
   }
}

void RenderRenderInfo::AccumulateModelVariants(ModelMap& m, om::ModelVariantsPtr model_variants, std::string const& current_variant)
{
   if (model_variants) {
      om::ModelLayerPtr layer = model_variants->GetVariant(current_variant);
      if (!layer) {
         layer = model_variants->GetVariant("default");
      }
      if (layer) {
         AccumulateModelVariant(m, layer);
      }
   }
}

void RenderRenderInfo::CheckMaterial(om::RenderInfoPtr render_info)
{
   std::string material_path = render_info->GetMaterial();
   if (material_path.empty()) {
      material_path = "materials/voxel.material.xml";
   }
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
   for (auto& entry : m) {
      for (int i = om::ModelLayer::NUM_LAYERS - 1; i >= 0; i--) {
         MatrixVector &v = entry.second.layers[i];
         if (!v.empty()) {
            flattened[entry.first] = std::move(v);
            break;
         }
      }
   }
}


void RenderRenderInfo::RemoveObsoleteNodes(FlatModelMap const& m)
{
   auto i = nodes_.begin();
   while (i != nodes_.end()) {
      auto j = m.find(i->first);
      if (j == m.end() || j->second != i->second.matrices) {
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
   // disambiguate them.  Ignore everything after the _ so we don't make authors manually
   // rename every single part when this happens.
   std::string bone = matrix_name;
   int pos = bone.find('_');
   if (pos != std::string::npos) {
      bone = bone.substr(0, pos);
   }
   return bone;
}

void RenderRenderInfo::AddModelNode(om::RenderInfoPtr render_info, std::string const& bone, MatrixVector const& matrices, float polygon_offset)
{
   ASSERT(nodes_.find(bone) == nodes_.end());

   csg::Point3f origin = csg::Point3f::zero;
   auto i = bones_offsets_.find(bone);
   if (i != bones_offsets_.end()) {
      origin = i->second;

      // 3DS Max uses a z-up, right handed origin....
      // The voxels in the matrix are stored y-up, left-handed...
      // Convert the max origin to y-up, right handed...
      csg::Point3f yUpOrigin(origin.x, origin.z, origin.y); 

      // Now convert to y-up, left-handed
      csg::Point3f yUpLHOrigin(yUpOrigin.x, yUpOrigin.y, -yUpOrigin.z);
      origin = yUpLHOrigin;
   }
   float scale = render_info->GetScale();

   auto& pipeline = Pipeline::GetInstance();
   H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone);
   
   ResourceCacheKey key;
   key.AddElement("origin", origin);
   for (voxel::QubicleMatrix const* matrix : matrices) {
      // this assumes matrices are loaded exactly once at a stable address.
      key.AddElement("matrix", matrix);
   }

   auto generate_matrix = [&matrices, &origin, &pipeline](csg::mesh_tools::mesh &mesh) {
      csg::Region3 all_models;
      for (voxel::QubicleMatrix const* matrix : matrices) {
         csg::Region3 model = voxel::QubicleBrush(matrix)
            .SetOffsetMode(voxel::QubicleBrush::File)
            .PaintOnce();
         all_models += model;
      }
      csg::RegionToMesh(all_models, mesh, -origin);
   };

   H3DNodeUnique node = pipeline.AddSharedMeshNode(parent, key, material_path_, generate_matrix);

   h3dSetNodeParamI(node.get(), H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(node.get(), H3DModel::PolygonOffsetF, 0, polygon_offset * 0.04f);
   h3dSetNodeParamF(node.get(), H3DModel::PolygonOffsetF, 1, polygon_offset * 10.0f);
   h3dSetNodeTransform(node.get(), 0, 0, 0, 0, 0, 0, scale, scale, scale);
   nodes_[bone] = NodeMapEntry(matrices, node);
}

void RenderRenderInfo::AddMissingNodes(om::RenderInfoPtr render_info, FlatModelMap const& m)
{
   float offset = 0;
   auto i = m.begin();
   while (i != m.end()) {
      auto j = nodes_.find(i->first);
      // xxx: what's with the std::vector compare in this if?  is that actually kosher?
      if (j == nodes_.end() || i->second != j->second.matrices) {
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
            CheckMaterial(render_info);
            RebuildModels(render_info);
            SetDirtyBits(SCALE_DIRTY);
         }
         if (dirty_ & SCALE_DIRTY) {
            float scale = render_info->GetScale();
            Skeleton& skeleton = entity_.GetSkeleton();
            skeleton.SetScale(scale);

            for (auto const& entry : nodes_) {
               H3DNode node = entry.second.node.get();
               float tx, ty, tz, rx, ry, rz, sx, sy, sz;

               h3dGetNodeTransform(node, &tx, &ty, &tz, &rx, &ry, &rz, &sx, &sy, &sz);
               tx *= (scale / sx);
               ty *= (scale / sy);
               tz *= (scale / sz);
               h3dSetNodeTransform(node, tx, ty, tz, rx, ry, rz, scale, scale, scale);
            }
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
