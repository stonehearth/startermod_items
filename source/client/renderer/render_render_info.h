#ifndef _RADIANT_CLIENT_RENDER_RENDER_INFO_H
#define _RADIANT_CLIENT_RENDER_RENDER_INFO_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "dm/set.h"
#include "h3d_resource_types.h"
#include "render_component.h"
#include "om/components/render_info.ridl.h"
#include "om/components/model_layer.ridl.h"
#include "om/components/model_variants.ridl.h"
#include "lib/voxel/forward_defines.h"
#include "render_node.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderRenderInfo : public RenderComponent
{
public:
   RenderRenderInfo(RenderEntity& entity, om::RenderInfoPtr render_info);
   ~RenderRenderInfo();

   void SetModelVariantOverride(bool enabled, std::string const& variant);
   void SetMaterialOverride(std::string const& materialOverride);

private:
   enum DirtyBits {
      ANIMATION_TABLE_DIRTY  = (1 << 0),
      MODEL_DIRTY            = (1 << 1),
      SCALE_DIRTY            = (1 << 2),
      MATERIAL_DIRTY         = (1 << 3)
   };

   typedef std::vector<voxel::QubicleMatrix const*> MatrixVector;
   struct ModelMapEntry {
      MatrixVector layers[om::ModelLayer::NUM_LAYERS];
   };
   typedef std::unordered_map<std::string, ModelMapEntry> ModelMap;
   typedef std::unordered_map<std::string, MatrixVector> FlatModelMap;

   struct NodeMapEntry {
      NodeMapEntry() : node(0) { }
      NodeMapEntry(MatrixVector const& v, RenderNode n) : matrices(v), node(n) { }

      MatrixVector         matrices;
      RenderNode           node;
   };

   typedef std::unordered_map<std::string, NodeMapEntry> NodeMap;
   typedef std::unordered_map<std::string, std::shared_ptr<voxel::QubicleFile>> QubicleFileMap;
   typedef std::unordered_map<std::string, csg::Point3f> BoneOffsetMap;

private:
   void AccumulateModelVariant(ModelMap& m, om::ModelLayerPtr layer);
   void AccumulateModelVariants(ModelMap& m, om::ModelVariantsPtr model_variants, std::string const& current_variant);
   void RebuildModels(om::RenderInfoPtr render_info);
   void CheckMaterial(om::RenderInfoPtr render_info);
   void FlattenModelMap(ModelMap& m, FlatModelMap& flattened);
   void RemoveObsoleteNodes(FlatModelMap const& m);
   std::string GetBoneName(std::string const& matrix_name);
   void AddModelNode(om::RenderInfoPtr render_info, std::string const& bone, MatrixVector const& matrices, float offset);
   void AddMissingNodes(om::RenderInfoPtr render_info, FlatModelMap const& m);
   void RebuildBoneOffsets(om::RenderInfoPtr render_info);
   void UpdateNextFrame();
   void Update();
   void SetDirtyBits(int flags);
   std::string GetModelVariant(om::RenderInfoPtr render_info) const;
   void ReApplyMaterial();

private:
   RenderEntity&           entity_;
   int                     dirty_;
   om::RenderInfoRef       render_info_;
   core::Guard             renderer_frame_guard_;
   dm::TracePtr            variant_trace_;
   dm::TracePtr            scale_trace_;
   dm::TracePtr            model_variant_trace_;
   dm::TracePtr            attached_trace_;
   dm::TracePtr            material_trace_;
   NodeMap                 nodes_;
   BoneOffsetMap           bones_offsets_;
   std::string             model_variant_override_;
   std::string             material_path_;
   SharedMaterial          override_material_;
   bool                    use_model_variant_override_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_RENDER_INFO_H
