#ifndef _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H
#define _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H

#include <unordered_map>
#include "namespace.h"
#include "math3d.h"
#include "om/om.h"
#include "dm/dm.h"
#include "types.h"
#include "render_component.h"
#include "skeleton.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;
class RenderAspect;

class RenderEntity
{
   public:
      RenderEntity(H3DNode parent, om::EntityPtr obj);
      ~RenderEntity();

      void Show(bool show);
      void SetSelected(bool selected);

      om::EntityId GetEntityId() const;
      void SetParent(H3DNode node);
      H3DNode GetParent() const; 
      H3DNode GetNode() const;
      std::string GetName() const;
      bool ShowDebugRegions() const;

      static int GetTotalObjectCount();

      Skeleton& GetSkeleton() { return skeleton_; }

      void SetDisplayIconicOveride(bool value);
      void ClearDisplayIconicOveride();

   private:
      void LoadAspects(om::EntityPtr obj);
      void Move(bool snap);
      void ModifyContents();
      void RemoveChildren();
      void AddChild(om::EntityPtr child);
      void RemoveChild(om::EntityPtr child);
      void MoveSceneNode(H3DNode node, const math3d::transform& transform, float scale = 1.0f);
      void UpdateNodeFlags();
      void UpdateComponents();
      void AddComponent(dm::ObjectType key, std::shared_ptr<dm::Object> value);
      void RemoveComponent(dm::ObjectType key);
      void OnSelected(om::Selection& sel, const math3d::ray3& ray,
                      const math3d::point3& intersection, const math3d::point3& normal);
      void CreateRenderRigs();

   protected:
      static int                          totalObjectCount_;

protected:
      typedef std::unordered_map<dm::ObjectType, std::shared_ptr<RenderComponent>> ComponentMap;
      H3DNode           node_;
      om::EntityRef     entity_;
      dm::GuardSet      tracer_;
      Skeleton          skeleton_;
      ComponentMap      components_;
      bool              initialized_;
      int               displayIconic_;
};

typedef std::shared_ptr<RenderEntity>  RenderEntityPtr;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDER_ENTITY_H
