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
      RenderEntity(H3DNode parent, om::EntityPtr obj);
      ~RenderEntity();

      void FinishConstruction();

      void Show(bool show);
      void SetSelected(bool selected);

      dm::ObjectId GetObjectId() const;
      om::EntityPtr GetEntity() const { return entity_.lock(); }
      void SetParent(H3DNode node);
      H3DNode GetParent() const; 
      H3DNode GetNode() const;
      std::string GetName() const;

      static int GetTotalObjectCount();

      Skeleton& GetSkeleton() { return skeleton_; }

      void SetModelVariantOverride(bool enabled, std::string const& variant);

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
      void AddComponent(dm::ObjectType key, std::shared_ptr<dm::Object> value);
      void AddLuaComponents(om::LuaComponentsPtr lua_components);
      void RemoveComponent(dm::ObjectType key);
      void OnSelected(om::Selection& sel, const csg::Ray3& ray,
                      const csg::Point3f& intersection, const csg::Point3f& normal);

   protected:
      static int                          totalObjectCount_;

protected:
      typedef std::unordered_map<dm::ObjectType, std::shared_ptr<RenderComponent>> ComponentMap;
      typedef std::unordered_map<std::string, luabind::object> LuaComponentMap;
      std::string       node_name_;
      H3DNodeUnique     node_;
      om::EntityRef     entity_;
      Skeleton          skeleton_;
      ComponentMap      components_;
      LuaComponentMap   lua_components_;
      LuaComponentMap   lua_invariants_;
      bool              initialized_;
      core::Guard       renderer_guard_;
      dm::TracePtr      components_trace_;
};

typedef std::shared_ptr<RenderEntity>  RenderEntityPtr;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H
