#ifndef _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H
#define _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H

#include <unordered_map>
#include "namespace.h"
#include "csg/transform.h"
#include "csg/ray.h"
#include "om/om.h"
#include "dm/dm.h"
#include "render_component.h"
#include "skeleton.h"
#include "core/guard.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;
class RenderAspect;

class RenderEntity : public std::enable_shared_from_this<RenderEntity>
{
   public:
      enum QueryFlags {
         UNSELECTABLE = (1 << 0)
      };

   public:
      RenderEntity(H3DNode parent, om::EntityPtr obj);
      ~RenderEntity();

      void FinishConstruction();
      void Destroy();

      void SetSelected(bool selected);

      dm::ObjectId GetObjectId() const;
      om::EntityPtr GetEntity() const { return entity_.lock(); }
      void SetParent(H3DNode node);

      void SetParentOverride(bool enabled);
      bool GetParentOverride() const;
      
      H3DNode GetParent() const; 
      H3DNode GetNode() const;
      H3DNode GetOriginNode() const;

      std::string const& GetName() const;
      std::string const GetMaterialPathFromKind(std::string const& matKind) const;

      static int GetTotalObjectCount();

      Skeleton& GetSkeleton() { return skeleton_; }

	  std::shared_ptr<RenderComponent> GetComponent(std::string const& name) const;

      void AddQueryFlag(int flag);
      void RemoveQueryFlag(int flag);
      bool HasQueryFlag(int flag) const;

      void ForAllSceneNodes(std::function<void(H3DNode node)> const& fn);

      void SetModelVariantOverride(std::string const& variant);
      void SetMaterialOverride(std::string const& overrideKind);
      void SetVisibleOverride(bool visible);

      bool GetVisibleOverride() const;
      std::string const& GetMaterialOverride() const;
      std::string const& GetModelVariantOverride() const;

   private:
      void LoadAspects(om::EntityPtr obj);
      void Move(bool snap);
      void ModifyContents();
      void RemoveChildren();
      void AddChild(om::EntityPtr child);
      void RemoveChild(om::EntityPtr child);
      void MoveSceneNode(H3DNode node, const csg::Transform& transform, float scale = 1.0f);
      void UpdateNodeFlags();
      void UpdateInvariantRenderers();
      void AddComponent(std::string const& key, std::shared_ptr<dm::Object> value);
      void AddLuaComponent(std::string const& key, om::DataStorePtr obj);
      void RemoveComponent(std::string const& key);
      void ForAllSceneNodes(H3DNode node, std::function<void(H3DNode node)> const& fn);
      void SetRenderInfoDirtyBits(int bits);

   protected:
      static int                          totalObjectCount_;

protected:
      typedef std::unordered_map<std::string, std::shared_ptr<RenderComponent>> ComponentMap;
      typedef std::unordered_map<std::string, luabind::object> LuaComponentMap;
      std::string       node_name_;
      H3DNode           node_;
      H3DNode           offsetNode_;
      om::EntityRef     entity_;
      dm::ObjectId      entity_id_;
      Skeleton          skeleton_;
      ComponentMap      components_;
      LuaComponentMap   lua_invariants_;
      bool              initialized_;
      core::Guard       selection_guard_;
      dm::TracePtr      components_trace_;
      dm::TracePtr      lua_components_trace_;
      uint32            query_flags_;
      std::string       model_variant_override_;
      std::string       material_override_;
      bool              visible_override_;
      bool              _parentOverride;
};

typedef std::shared_ptr<RenderEntity>  RenderEntityPtr;
typedef std::weak_ptr<RenderEntity>  RenderEntityRef;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H
