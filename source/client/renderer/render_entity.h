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
#include "resources/animation.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;
class RenderAspect;
class RenderEntity;

typedef std::shared_ptr<RenderEntity> RenderEntityPtr;
typedef std::weak_ptr<RenderEntity> RenderEntityRef;

class RenderEntity : public std::enable_shared_from_this<RenderEntity>
{
   public:
      class VisibilityHandle
      {
         public:
            VisibilityHandle(RenderEntityRef renderEntityRef);
            ~VisibilityHandle();
            void SetVisible(bool visible);
            bool GetVisible();
            void Destroy();

         private:
            RenderEntityRef renderEntityRef_;
            bool visible_;
      };
      typedef std::shared_ptr<VisibilityHandle> VisibilityHandlePtr;

      enum QueryFlags {
         UNSELECTABLE = (1 << 0)
      };

   public:
      RenderEntity(H3DNode parent, om::EntityPtr obj);
      ~RenderEntity();

      bool IsValid() const;
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
      std::string GetMaterialPathFromKind(std::string const& matKind, std::string const& deflt) const;

      static int GetTotalObjectCount();

      Skeleton& GetSkeleton() { return skeleton_; }

      std::shared_ptr<RenderComponent> GetComponentRenderer(core::StaticString name) const;

      void AddQueryFlag(int flag);
      void RemoveQueryFlag(int flag);
      bool HasQueryFlag(int flag) const;

      void ForAllSceneNodes(std::function<void(H3DNode node)> const& fn);

      std::string const& GetModelVariantOverride() const;
      void SetModelVariantOverride(std::string const& variant);

      std::string const& GetMaterialOverride() const;
      void SetMaterialOverride(std::string const& overrideKind);

      bool GetVisibleOverride() const;
      VisibilityHandlePtr GetVisibilityOverrideHandle();

      void Pose(const char* animationName, int time);
      void Pose(res::AnimationPtr const& animation, int offset);
      res::AnimationPtr GetAnimation(const char* animationName);

   private:
      void SetVisibleOverride(bool visible);
      void LoadAspects(om::EntityPtr obj);
      void Move(bool snap);
      void ModifyContents();
      void RemoveChildren();
      void AddChild(om::EntityPtr child);
      void RemoveChild(om::EntityPtr child);
      void UpdateNodeFlags();
      void UpdateInvariantRenderers();
      void AddComponent(core::StaticString key, std::shared_ptr<dm::Object> value);
      void AddLuaComponent(core::StaticString key, om::DataStorePtr obj);
      void RemoveComponent(core::StaticString key);
      void ForAllSceneNodes(H3DNode node, std::function<void(H3DNode node)> const& fn);
      void SetRenderInfoDirtyBits(int bits);

   protected:
      static int                          totalObjectCount_;

protected:
      typedef std::unordered_map<core::StaticString, std::shared_ptr<RenderComponent>, core::StaticString::Hash> ComponentMap;
      typedef std::unordered_map<core::StaticString, luabind::object, core::StaticString::Hash> LuaComponentMap;
      std::string       node_name_;
      H3DNode           node_;
      H3DNode           offsetNode_;
      om::EntityRef     entity_;
      dm::ObjectId      entity_id_;
      Skeleton          skeleton_;
      ComponentMap      components_;
      LuaComponentMap   lua_invariants_;
      bool              initialized_;
      bool              destroyed_;
      core::Guard       selection_guard_;
      dm::TracePtr      components_trace_;
      dm::TracePtr      lua_components_trace_;
      uint32            query_flags_;
      std::string       model_variant_override_;
      std::string       material_override_;
      uint32            visible_override_ref_count_;
      bool              parentOverride_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H
