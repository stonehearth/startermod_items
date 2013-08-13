#ifndef _RADIANT_PHYSICS_OCTTREE_H
#define _RADIANT_PHYSICS_OCTTREE_H

#include <unordered_map>
#include "math3d.h"
#include "namespace.h"
#include "csg/region.h"
#include "om/om.h"
#include "dm/dm.h"
#include "nav_grid.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class OctTree {
   public:
      OctTree();

      void SetRootEntity(om::EntityPtr);
      void Cleanup();

      void Update(int now);

      typedef std::function<bool (om::EntityPtr )>   QueryCallback;
      typedef std::function<void (om::EntityPtr, float)> RayQueryCallback;

      void GetActorsIn(const math3d::aabb &bounds, QueryCallback cb);      
      void TraceRay(const math3d::ray3 &ray, RayQueryCallback cb);

      bool IsPassable(const math3d::ipoint3& at) const;
      bool CanStand(const math3d::ipoint3& at) const { return navgrid_.CanStand(at); }
      
      bool IsStuck(om::EntityPtr object);
      void Unstick(om::EntityPtr object);
      void Unstick(std::vector<math3d::ipoint3> &points);
      math3d::ipoint3 Unstick(const math3d::ipoint3 &pt);

      // Path finding helpers
      std::vector<std::pair<math3d::ipoint3, int>> ComputeNeighborMovementCost(const math3d::ipoint3& from) const;
      int EstimateMovementCost(const math3d::ipoint3& src, const math3d::ipoint3& dst) const;
      int EstimateMovementCost(const math3d::ipoint3& src, const std::vector<math3d::ipoint3>& points) const;
      int EstimateMovementCost(const math3d::ipoint3& src, const csg::Region3& dst) const;

   protected:
      bool Intersects(math3d::aabb bounds, om::RegionCollisionShapePtr rgnCollsionShape) const;   
      bool CanStepUp(math3d::ipoint3& at) const;
      bool CanFallDown(math3d::ipoint3& at) const;

      void FilterAllActors(std::function <bool(om::EntityPtr)> filter);
      om::EntityPtr FindFirstActor(om::EntityPtr root, std::function <bool(om::EntityPtr)> filter);

   private:
      void TraceEntity(om::EntityPtr entity);
      void OnComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component);
      void TraceSensor(om::SensorPtr sensor);
      void UpdateSensors();
      bool UpdateSensor(om::SensorPtr sensor);
      void UpdateEntityContainer(om::EntityContainerPtr container);
    
   protected:

      std::map<dm::ObjectId, om::EntityRef>     entities_;
      std::map<dm::ObjectId, om::SensorRef>     sensors_;
      dm::Guard                                 guards_;
      NavGrid                                   navgrid_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_OCTTREE_H
 