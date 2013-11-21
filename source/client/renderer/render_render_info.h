#ifndef _RADIANT_CLIENT_RENDER_RENDER_INFO_H
#define _RADIANT_CLIENT_RENDER_RENDER_INFO_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "dm/set.h"
#include "h3d_resource_types.h"
#include "render_component.h"
#include "om/components/render_info.h"
#include "lib/voxel/forward_defines.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderRenderInfo : public RenderComponent
{
public:
   RenderRenderInfo(RenderEntity& entity, om::RenderInfoPtr render_info);
   ~RenderRenderInfo();

   void SetModelVariantOverride(bool enabled, std::string const& variant);
private:

private:
   enum DirtyBits {
      ANIMATION_TABLE_DIRTY  = (1 << 0),
      MODEL_DIRTY            = (1 << 1),
      SCALE_DIRTY            = (1 << 2)
   };

   struct ModelMapEntry {
      ModelMapEntry() {
         for (int i = 0; i < om::ModelVariant::NUM_LAYERS; i++) {
            layers[i] = nullptr;
         }
      }
      voxel::QubicleMatrix const* layers[om::ModelVariant::NUM_LAYERS];
   };
   typedef std::unordered_map<std::string, ModelMapEntry> ModelMap;
   typedef std::unordered_map<std::string, voxel::QubicleMatrix const*> FlatModelMap;

   struct NodeMapEntry {
      NodeMapEntry() : matrix(nullptr), node(0) { }
      NodeMapEntry(voxel::QubicleMatrix const* m, H3DNodeUnique n) : matrix(m), node(n) { }

      voxel::QubicleMatrix const* matrix;
      H3DNodeUnique        node;
   };

   typedef std::unordered_map<std::string, NodeMapEntry> NodeMap;
   typedef std::unordered_map<std::string, std::shared_ptr<voxel::QubicleFile>> QubicleFileMap;
   typedef std::unordered_map<std::string, csg::Point3f> BoneOffsetMap;

private:
   void AccumulateModelVariant(ModelMap& m, om::ModelVariantPtr v);
   void AccumulateModelVariants(ModelMap& m, om::ModelVariantsPtr model_variants, std::string const& current_variant);
   void RebuildModels(om::RenderInfoPtr render_info);
   void CheckMaterial(om::RenderInfoPtr render_info);
   void FlattenModelMap(ModelMap& m, FlatModelMap& flattened);
   void RemoveObsoleteNodes(FlatModelMap const& m);
   std::string GetBoneName(std::string const& matrix_name);
   void AddModelNode(om::RenderInfoPtr render_info, std::string const& bone, voxel::QubicleMatrix const* matrix, float offset);
   void AddMissingNodes(om::RenderInfoPtr render_info, FlatModelMap const& m);
   void RebuildBoneOffsets(om::RenderInfoPtr render_info);
   void UpdateNextFrame();
   void Update();
   void SetDirtyBits(int flags);
   std::string GetModelVariant(om::RenderInfoPtr render_info) const;

private:
   static std::shared_ptr<voxel::QubicleFile> LoadQubicleFile(std::string const& uri);
   static QubicleFileMap   qubicle_map__;

private:
   RenderEntity&           entity_;
   int                     dirty_;
   om::RenderInfoRef       render_info_;
   core::Guard             renderer_frame_trace_;
   core::Guard             variant_guards_;
   core::Guard             render_info_guards_;
   NodeMap                 nodes_;
   BoneOffsetMap           bones_offsets_;
   std::string             model_variant_override_;
   H3DResUnique            material_;
   std::string             material_path_;
   std::string             model_mode_;
   bool                    use_model_variant_override_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_RENDER_INFO_H
