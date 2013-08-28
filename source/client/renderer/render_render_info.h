#ifndef _RADIANT_CLIENT_RENDER_RENDER_INFO_H
#define _RADIANT_CLIENT_RENDER_RENDER_INFO_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "dm/set.h"
#include "types.h"
#include "render_component.h"
#include "om/components/render_info.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;
class QubicleFile;
class QubicleMatrix;

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
      QubicleMatrix const* layers[om::ModelVariant::NUM_LAYERS];
   };
   typedef std::unordered_map<std::string, ModelMapEntry> ModelMap;
   typedef std::unordered_map<std::string, QubicleMatrix const*> FlatModelMap;

   struct NodeMapEntry {
      NodeMapEntry() : matrix(nullptr), node(0) { }
      NodeMapEntry(QubicleMatrix const* m, H3DNode n) : matrix(m), node(n) { }

      QubicleMatrix const* matrix;
      RenderNode           node;
   };

   typedef std::unordered_map<std::string, NodeMapEntry> NodeMap;
   typedef std::unordered_map<std::string, std::shared_ptr<QubicleFile>> QubicleFileMap;
   typedef std::unordered_map<std::string, csg::Point3f> BoneOffsetMap;

private:
   void AccumulateModelVariant(ModelMap& m, om::ModelVariantPtr v);
   void AccumulateModelVariants(ModelMap& m, om::ModelVariantsPtr model_variants, std::string const& current_variant);
   void RebuildModels(om::RenderInfoPtr render_info);
   void FlattenModelMap(ModelMap& m, FlatModelMap& flattened);
   void RemoveObsoleteNodes(FlatModelMap const& m);
   std::string GetBoneName(std::string const& matrix_name);
   void AddModelNode(om::RenderInfoPtr render_info, std::string const& bone, QubicleMatrix const* matrix);
   void AddMissingNodes(om::RenderInfoPtr render_info, FlatModelMap const& m);
   void RebuildBoneOffsets(om::RenderInfoPtr render_info);
   void Update();
   std::string GetModelVariant(om::RenderInfoPtr render_info) const;

private:
   static QubicleFile const& LoadQubicleFile(std::string const& uri);
   static QubicleFileMap   qubicle_map__;

private:
   RenderEntity&           entity_;
   int                     dirty_;
   om::RenderInfoRef       render_info_;
   dm::Guard               renderer_frame_trace_;
   dm::Guard               variant_guards_;
   dm::Guard               render_info_guards_;
   NodeMap                 nodes_;
   BoneOffsetMap           bones_offsets_;
   std::string             model_variant_override_;
   bool                    use_model_variant_override_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_RENDER_INFO_H
