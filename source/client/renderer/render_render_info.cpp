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
#include "om/components/mob.ridl.h"
#include "Horde3DUtils.h"
#include "resources/res_manager.h"
#include "lib/perfmon/perfmon.h"
#include "lib/voxel/qubicle_file.h"
#include "lib/voxel/qubicle_brush.h"
#include "pipeline.h"
#include "geometry_info.h"
#include "csg/region_tools.h"
#include "Horde3D.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define RI_LOG(level)      LOG(renderer.render_info, level)

const char* defaultMaterial = "materials/voxel.material.json";

RenderRenderInfo::RenderRenderInfo(RenderEntity& entity, om::RenderInfoPtr render_info) :
   entity_(entity),
   render_info_(render_info),
   scale_(1.0),
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
      scale_trace_ = render_info->TraceScale("RenderRenderInfo scale", dm::RENDER_TRACES)
                                    ->OnModified(set_scale_dirty_bit);

      visible_trace_ = render_info->TraceVisible("RenderRenderInfo visible", dm::RENDER_TRACES)
                                    ->OnModified(set_visible_dirty_bit);

      // if the variant we want to render changes...
      model_variant_trace_ = render_info->TraceModelVariant("RenderRenderInfo model variant", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);

      // if any entry in the attached items thing changes...
      attached_trace_ = render_info->TraceAttachedEntities("RenderRenderInfo attached entities", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);

      // if the material changes...
      material_trace_ = render_info->TraceMaterial("RenderRenderInfo material", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);
      material_map_trace_ = render_info->TraceMaterialMaps("RenderRenderInfo material maps", dm::RENDER_TRACES)->OnModified(set_model_dirty_bit);
   }

   // Manually update immediately on construction.
   dirty_ = -1;
   Update();
}

RenderRenderInfo::~RenderRenderInfo()
{
   DestroyVoxelMeshNode();
}


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

void RenderRenderInfo::AccumulateModelVariant(ModelMap& m, om::ModelLayerPtr layer)
{
   if (layer) {
      om::ModelLayer::Layer level = layer->GetLayer();

      variant_trace_ = layer->TraceModels("RenderRenderInfo models", dm::RENDER_TRACES)
                              ->OnModified([this]() {
                                 SetDirtyBits(MODEL_DIRTY);
                              });

      for (std::string const& model : layer->EachModel()) {
         voxel::QubicleFile const* qubicle;
         try {
            qubicle = res::ResourceManager2::GetInstance().LookupQubicleFile(model);
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

   H3DNode node = entity_.GetOriginNode();
   if (visible) {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, false, false);
   } else {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true, false);
   }
}

void RenderRenderInfo::CheckScale(om::RenderInfoPtr render_info)
{
   float scale = render_info ? render_info->GetScale() : 1.0f;

   if (scale != scale_) {
      scale_ = scale;
      entity_.GetSkeleton().SetScale(scale);

      h3dSetNodeParamF(entity_.GetNode(), H3DModel::ModelScaleF, 0, scale);
   }
}

void RenderRenderInfo::CheckMaterial(om::RenderInfoPtr render_info)
{
   // First, check to see if we have manually overridden the material for this entity.
   std::string material = entity_.GetMaterialOverride();
   if (!material.empty()) {
      ApplyMaterialToVoxelNodes(material.c_str());
      return;
   }

   // No override.  Try the material in the render_info
   if (render_info) {
      material = render_info->GetMaterial();
      if (!material.empty()) {
         ApplyMaterialToVoxelNodes(material.c_str());
         return;
      }
   }

   // Use the color map version.
   for (VoxelNode n: _voxelMeshNodes) {
      H3DRes mat = h3dAddResource(H3DResTypes::Material, n.material, 0);
      h3dSetNodeParamI(n.node, H3DVoxelMeshNodeParams::MatResI, mat);
   }
}

void RenderRenderInfo::ApplyMaterialToVoxelNodes(core::StaticString material)
{
   H3DRes mat = h3dAddResource(H3DResTypes::Material, material, 0);
   for (VoxelNode n: _voxelMeshNodes) {
      h3dSetNodeParamI(n.node, H3DVoxelMeshNodeParams::MatResI, mat);
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
      RebuildMaterialMap(render_info);
      RebuildModel(render_info, flattened);
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


std::string RenderRenderInfo::GetBoneName(std::string const& matrix_name)
{
   // Qubicle requires that ever matrix in the file have a unique name.  While authoring,
   // if you copy/pasta a matrix, it will rename it from matrixName to matrixName_2 to
   // disambiguate them.  Ignore everything after the _ so we don't make authors manually
   // rename every single part when this happens.
   std::string bone = matrix_name;
   int pos = (int)bone.find('_');
   if (pos != std::string::npos) {
      bone = bone.substr(0, pos);
   }
   return bone;
}

void RenderRenderInfo::RebuildMaterialMap(om::RenderInfoPtr render_info)
{
   _materialMap.clear();

   if (render_info) {
      std::string colormap = render_info->GetColorMap();
      if (colormap.empty()) {
         return;
      }
      InverseColorMap icm = CreateInverseColorMap(colormap);
      auto icmEnd = icm.end();

      for (std::string const& materialMap : render_info->EachMaterialMap()) {
         try {
            res::ResourceManager2::GetInstance().LookupJson(materialMap, [this, &icm, icmEnd](JSONNode const& data) {
               for (json::Node const& node : json::Node(data)) {
                  std::string material = node.get<std::string>("render_material", "");
                  if (!material.empty()) {
                     csg::MaterialName materialName(material);

                     // node.name() is the name of the material.  we can find all the colors mapped
                     // to this material by looking them up in the inverse color map
                     auto i = icm.find(node.name());
                     if (i != icmEnd) {
                        for (csg::Color3 const& color : i->second) {
                           _materialMap.emplace(color, materialName);
                        }
                     }
                  }
               }
            });
         } catch (res::Exception const& e) {
            RI_LOG(1) << "failed to process material map " << materialMap << " (" << e.what() << ")";
         }
      }
   }
}

RenderRenderInfo::InverseColorMap RenderRenderInfo::CreateInverseColorMap(std::string const& colormap)
{
   InverseColorMap icm;

   try {
      res::ResourceManager2::GetInstance().LookupJson(colormap, [&icm](JSONNode const& cm) mutable {
         for (json::Node const& i : json::Node(cm)) {
            // i.name() is the color (e.g. #ff0000).
            // *i is the material map entry (e.g. 'stonehearth:materials:wood_1')
            csg::Color3 color = csg::Color3::FromString(i.name());
            icm[i.as<csg::MaterialName>()].emplace_back(color);
         }
      });
   } catch (res::Exception const& e) {
      RI_LOG(1) << "failed to process colormap " << colormap << " (" << e.what() << ")";
   }
   return icm;
}

void RenderRenderInfo::RebuildModel(om::RenderInfoPtr render_info, FlatModelMap const& nodes)
{
   ASSERT(render_info);

   DestroyVoxelMeshNode();

   //RI_LOG(7) << "adding model node for bone " << bone;
   ResourceCacheKey key;

   bool useSkeletonOrigin = !render_info->GetAnimationTable().empty();
   key.AddElement("useSkeletonOrigin", useSkeletonOrigin); 

   Skeleton& skeleton = entity_.GetSkeleton();
   for (auto const& node : nodes) {
      std::string const& boneName = node.first;
      MatrixVector const& matrices = node.second;

      // This 'primes' the skeleton, making sure every single bone node is allocated before mesh
      // generation begins.  This is done to ensure identical mapping when the geometry is created.
      int boneNum = skeleton.GetBoneNumber(boneName);

      for (voxel::QubicleMatrix const* matrix : matrices) {
         // this assumes matrices are loaded exactly once at a stable address.
         key.AddElement("matrix", matrix);
         key.AddElement("matrix", boneNum);
      }
   }

   auto generate_matrix = [this, useSkeletonOrigin, &skeleton, &nodes](csg::MaterialToMeshMap& meshes, int lodLevel) {
      for (auto const& node : nodes) {
         std::string const& boneName = node.first;
         MatrixVector const& matrices = node.second;

         if (matrices.empty()) {
            return;
         }
         csg::Region3 all_models;

         // Since we're stacking them up and deriving the position of the generated mesh from the
         // same skeleton, we try to make very sure that they all have the exact same matrix
         // proportions.  If not, complain loudly.
         csg::Point3 size = matrices.front()->GetSize();
         csg::Point3 pos  = matrices.front()->GetPosition();

         for (voxel::QubicleMatrix const* matrix : matrices) {
            csg::Region3 model = voxel::QubicleBrush(matrix, lodLevel)
               .SetOffsetMode(voxel::QubicleBrush::File)
               .PaintOnce();

            if (matrix->GetSize() != size) {
               RI_LOG(0) << "stacked matrix " << matrix->GetName() << " size " << matrix->GetSize() << " does not match first matrix size of " << size;
            }
            if (matrix->GetPosition() != pos) {
               RI_LOG(0) << "stacked matrix " << matrix->GetName() << " pos " << matrix->GetPosition() << " does not match first matrix pos of " << pos;
            }
            all_models += model;   
         }

         csg::Point3f origin = csg::Point3f::zero;
         if (useSkeletonOrigin) {
            origin = GetBoneOffset(boneName);
         }

         // Qubicle orders voxels in the file as if we were looking at the model from the
         // front.  The matrix loader will reverse them when covering to a region, but this
         // means the origin contained in the skeleton file is now reading from the wrong
         // "side" of the model (like how your sides get reversed in a mirror).  Flip it over
         // to the other side before meshing.
         csg::Point3f meshOrigin = origin;
         meshOrigin.x = (float)pos.x * 2 + size.x - meshOrigin.x;

         RI_LOG(7) << "offsetting mesh " << all_models.GetBounds() << " origin:" << origin << " meshOrigin:" << meshOrigin << " matrixSize:" << size << " pos:" << pos;

         csg::RegionToMeshMap(all_models, meshes, -meshOrigin, _materialMap, defaultMaterial, skeleton.GetBoneNumber(boneName));
      }
   };
   // If the thing has bones, we assume it has an animation.  If it has an animation, we
   // turn off LOD and instancing.
   bool hasBones = skeleton.GetNumBones() > 1;
   bool noInstancing = hasBones;
   int lodCount = hasBones ? 1 : 4;

   Pipeline::MaterialToGeometryMapPtr geometry;
   Pipeline& pipeline = Pipeline::GetInstance();

   if (render_info->GetCacheModelGeometry()) {
      pipeline.CreateSharedGeometryFromGenerator(geometry, key, _materialMap, generate_matrix, noInstancing, lodCount);
   } else {
      pipeline.CreateGeometryFromGenerator(geometry, _materialMap, generate_matrix, true, lodCount);
   }

   H3DNode parent = entity_.GetNode();
   for (auto const& entry : *geometry) {
      csg::MaterialName material = entry.first;
      GeometryInfo const& geo = entry.second;

      H3DNode mesh = pipeline.CreateVoxelMeshNode(parent, geo);
      _voxelMeshNodes.emplace_back(mesh, material);
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

csg::Point3f RenderRenderInfo::GetBoneOffset(std::string const& boneName)
{
   csg::Point3f offset = csg::Point3f::zero;

   auto i = bones_offsets_.find(boneName);
   if (i != bones_offsets_.end()) {
      offset = i->second;

      // The file format for animation is right-handed, z-up, with the model looking
      // down the -y axis.  We need y-up, looking down -z.  Since we don't care about
      // rotation and we need to preserve x, just flip em around.
      offset = csg::Point3f(offset.x, offset.z, offset.y);
   }

   return offset;
}

void RenderRenderInfo::DestroyVoxelMeshNode()
{
   for (VoxelNode& node : _voxelMeshNodes) {
      h3dRemoveNode(node.node);
   }
   _voxelMeshNodes.clear();  
}
