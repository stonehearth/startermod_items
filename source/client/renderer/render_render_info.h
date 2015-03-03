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

   enum DirtyBits {
      ANIMATION_TABLE_DIRTY  = (1 << 0),
      MODEL_DIRTY            = (1 << 1),
      SCALE_DIRTY            = (1 << 2),
      MATERIAL_DIRTY         = (1 << 3),
      VISIBLE_DIRTY          = (1 << 4),
   };
   void SetDirtyBits(int flags);

private:
   typedef std::vector<voxel::QubicleMatrix const*> MatrixVector;
   struct ModelMapEntry {
      MatrixVector layers[om::ModelLayer::NUM_LAYERS];
   };
   typedef std::unordered_map<std::string, ModelMapEntry> ModelMap;
   typedef std::unordered_map<std::string, MatrixVector> FlatModelMap;

   struct NodeMapEntry {
      NodeMapEntry() : node(0) { }
      NodeMapEntry(MatrixVector const& v, RenderNodePtr n) : matrices(v), node(n) { }

      MatrixVector         matrices;
      RenderNodePtr        node;
   };

   typedef std::unordered_map<std::string, MatrixVector> NodeMap;
   typedef std::unordered_map<std::string, std::shared_ptr<voxel::QubicleFile>> QubicleFileMap;
   typedef std::unordered_map<std::string, csg::Point3f> BoneOffsetMap;

private:
   void AccumulateModelVariant(ModelMap& m, om::ModelLayerPtr layer);
   void AccumulateModelVariants(ModelMap& m, om::ModelVariantsPtr model_variants, std::string const& current_variant);
   void RebuildModels(om::RenderInfoPtr render_info);
   void CheckMaterial(om::RenderInfoPtr render_info);
   void CheckScale(om::RenderInfoPtr render_info);
   void CheckVisible(om::RenderInfoPtr render_info);
   void FlattenModelMap(ModelMap& m, FlatModelMap& flattened);
   void RemoveObsoleteNodes(FlatModelMap const& m);
   std::string GetBoneName(std::string const& matrix_name);
   void RebuildModel(om::RenderInfoPtr render_info);
   void AddModelNode(om::RenderInfoPtr render_info, std::string const& bone, MatrixVector const& matrices, float offset);
   void RebuildBoneOffsets(om::RenderInfoPtr render_info);
   void UpdateNextFrame();
   void Update();
   std::string GetModelVariant(om::RenderInfoPtr render_info) const;
   void ReApplyMaterial();

private:
   RenderEntity&           entity_;
   int                     dirty_;
   float                   scale_;
   om::RenderInfoRef       render_info_;
   core::Guard             renderer_frame_guard_;
   dm::TracePtr            variant_trace_;
   dm::TracePtr            scale_trace_;
   dm::TracePtr            visible_trace_;
   dm::TracePtr            model_variant_trace_;
   dm::TracePtr            attached_trace_;
   dm::TracePtr            material_trace_;
   NodeMap                 nodes_;
   RenderNodePtr           render_node_;
   BoneOffsetMap           bones_offsets_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_RENDER_INFO_H
