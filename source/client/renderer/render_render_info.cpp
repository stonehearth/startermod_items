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
   dirty_(-1)
{
   auto set_scale_dirty_bit = [=]() {
      SetDirtyBits(SCALE_DIRTY);
   };
   auto set_model_dirty_bit = [=]() {
      SetDirtyBits(MODEL_DIRTY);
   };
   auto set_visible_dirty_bit = [=]() {
      SetDirtyBits(VISIBLE_DIRTY);
   };

   if (render_info) {
      scale_trace_ = render_info->TraceScale("render", dm::RENDER_TRACES)
                                    ->OnModified(set_scale_dirty_bit);

      visible_trace_ = render_info->TraceScale("render", dm::RENDER_TRACES)
                                    ->OnModified(set_visible_dirty_bit);

      // if the variant we want to render changes...
      model_variant_trace_ = render_info->TraceModelVariant("render", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);

      // if any entry in the attached items thing changes...
      attached_trace_ = render_info->TraceAttachedEntities("render", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);

      // if the material changes...
      material_trace_ = render_info->TraceMaterial("render", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);
   }

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

void RenderRenderInfo::CheckVisible(om::RenderInfoPtr render_info)
{
   bool visible = entity_.GetVisibleOverride();
   if (visible && render_info) {
      visible = render_info->GetVisible();
   }
   RI_LOG(9) << "updating visibility of " << *entity_.GetEntity() << " (visible: " << std::boolalpha << visible << ")";

   H3DNode node = entity_.GetNode();
   if (visible) {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, false, false);
   } else {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true, false);
   }
}

void RenderRenderInfo::CheckScale(om::RenderInfoPtr render_info)
{
   float scale = render_info ? render_info->GetScale() : 1.0f;
   Skeleton& skeleton = entity_.GetSkeleton();
   skeleton.SetScale(scale);

   for (auto const& entry : nodes_) {
      H3DNode node = entry.second.node->GetNode();
      float tx, ty, tz, rx, ry, rz, sx, sy, sz;

      h3dGetNodeTransform(node, &tx, &ty, &tz, &rx, &ry, &rz, &sx, &sy, &sz);
      tx *= (scale / sx);
      ty *= (scale / sy);
      tz *= (scale / sz);
      h3dSetNodeTransform(node, tx, ty, tz, rx, ry, rz, scale, scale, scale);
   }
}

void RenderRenderInfo::CheckMaterial(om::RenderInfoPtr render_info)
{
   std::string material = entity_.GetMaterialOverride();
   if (material.empty() && render_info) {
      material = render_info->GetMaterial();
   }
   if (material.empty()) {
      material = "materials/voxel.material.xml";
   }

   H3DRes mat = h3dAddResource(H3DResTypes::Material, material.c_str(), 0);
   if (mat != 0) {
      for (auto& entry : nodes_) {
         entry.second.node->SetMaterial(mat);
      }
   }
}

void RenderRenderInfo::RebuildModels(om::RenderInfoPtr render_info)
{
   om::EntityPtr entity = entity_.GetEntity();

   if (entity && render_info) {
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
   ASSERT(render_info);
   ASSERT(nodes_.find(bone) == nodes_.end());

   bool moveMatrixOrigin = false;
   csg::Point3f origin = render_info->GetModelOrigin();

   auto i = bones_offsets_.find(bone);
   if (i != bones_offsets_.end()) {
      origin = i->second;

      // The file format for animation is right-handed, z-up, with the model looking
      // down the -y axis.  We need y-up, looking down -z.  Since we don't care about
      // rotation and we need to preserve x, just flip em around.
      origin = csg::Point3f(origin.x, origin.z, origin.y);
      moveMatrixOrigin = true;
   }
   float scale = render_info->GetScale();

   H3DNode parent = entity_.GetSkeleton().GetSceneNode(bone);
   
   ResourceCacheKey key;
   key.AddElement("origin", origin);
   for (voxel::QubicleMatrix const* matrix : matrices) {
      // this assumes matrices are loaded exactly once at a stable address.
      key.AddElement("matrix", matrix);
   }

   auto generate_matrix = [&matrices, origin, moveMatrixOrigin](csg::mesh_tools::mesh &mesh, int lodLevel) {
      if (!matrices.empty()) {
         csg::Region3 all_models;
         // Since we're stacking them up and deriving the position of the generated mesh from the
         // same skeleton, we try to make very sure that they all have the exact same matrix
         // proportions.  If not, complain loudly.
         csg::Point3 size = matrices.front()->GetSize();

         for (voxel::QubicleMatrix const* matrix : matrices) {
            csg::Region3 model = voxel::QubicleBrush(matrix, lodLevel)
               .SetOffsetMode(voxel::QubicleBrush::File)
               .PaintOnce();

            if (matrix->GetSize() != size) {
               RI_LOG(0) << "stacked matrix " << matrix->GetName() << " size " << matrix->GetSize() << " does not match first matrix size of " << size;
            }
            all_models += model;   
         }
         // Qubicle orders voxels in the file as if we were looking at the model from the
         // front.  The matrix loader will reverse them when covering to a region, but this
         // means the origin contained in the skeleton file is now reading from the wrong
         // "side" of the model (like how your sides get reversed in a mirror).  Flip it over
         // to the other side before meshing.
         csg::Point3f meshOrigin = origin;
         if (moveMatrixOrigin) {
            meshOrigin.x = (float)size.x - origin.x;
         }
         csg::RegionToMesh(all_models, mesh, -meshOrigin, true);
      }
   };

   RenderNodePtr node = RenderNode::CreateSharedCsgMeshNode(parent, key, generate_matrix);

   h3dSetNodeParamI(node->GetNode(), H3DModel::PolygonOffsetEnabledI, 1);
   h3dSetNodeParamF(node->GetNode(), H3DModel::PolygonOffsetF, 0, polygon_offset * 0.04f);
   h3dSetNodeParamF(node->GetNode(), H3DModel::PolygonOffsetF, 1, polygon_offset * 10.0f);
   h3dSetNodeTransform(node->GetNode(), 0, 0, 0, 0, 0, 0, scale, scale, scale);
   nodes_[bone] = NodeMapEntry(matrices, node);
}

void RenderRenderInfo::AddMissingNodes(om::RenderInfoPtr render_info, FlatModelMap const& m)
{
   ASSERT(render_info);

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
   if (render_info) {
      bones_offsets_.clear();
      std::string const& animation_table_name = render_info->GetAnimationTable();
      if (!animation_table_name.empty()) {
         res::ResourceManager2::GetInstance().LookupJson(animation_table_name, [&](const json::Node& table) {
            for (const auto& entry : table.get("skeleton", JSONNode())) {
               csg::Point3f pt;
               for (int j = 0; j < 3; j++) {
                  pt[j] = json::Node(entry).get(j, 0.0f);
               }
               bones_offsets_[entry.name()] = pt;
            }
         });
      }
   }
}

void RenderRenderInfo::Update()
{
   if (dirty_) {
      auto render_info = render_info_.lock();
      RI_LOG(5) << "updating render_info for " << entity_.GetObjectId();
      if (dirty_ & ANIMATION_TABLE_DIRTY) {
         RebuildBoneOffsets(render_info);
         SetDirtyBits(MODEL_DIRTY);
      }
      if (dirty_ & MODEL_DIRTY) {
         RebuildModels(render_info);
         SetDirtyBits(MATERIAL_DIRTY | SCALE_DIRTY | VISIBLE_DIRTY);
      }
      if (dirty_ & MATERIAL_DIRTY) {
         CheckMaterial(render_info);
      }
      if (dirty_ & SCALE_DIRTY) {
         CheckScale(render_info);
      }
      if (dirty_ & VISIBLE_DIRTY) {
         CheckVisible(render_info);
      }
      dirty_ = 0;
   }
   renderer_frame_guard_.Clear();
}

std::string RenderRenderInfo::GetModelVariant(om::RenderInfoPtr render_info) const
{
   ASSERT(render_info);
   std::string variant = entity_.GetModelVariantOverride();
   if (variant.empty()) {
      variant = render_info->GetModelVariant();
   }
   return variant;
}
