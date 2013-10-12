#ifndef _RADIANT_PHYSICS_OCTTREE_H
#define _RADIANT_PHYSICS_OCTTREE_H

#include <unordered_map>
#include "namespace.h"
#include "csg/region.h"
#include "csg/ray.h"
#include "om/om.h"
#include "dm/dm.h"
#include "nav_grid.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class OctTree {
   public:
      OctTree();

      void SetRootEntity(om::EntityPtr);
      void Cleanup();

      NavGrid& GetNavGrid() { return navgrid_; } // sigh
      NavGrid const& GetNavGrid() const { return navgrid_; } // sigh

      void Update(int now);

      typedef std::function<bool (om::EntityPtr )>   QueryCallback;
      typedef std::function<void (om::EntityPtr, float)> RayQueryCallback;

      void GetActorsIn(const csg::Cube3f &bounds, QueryCallback cb);      
      void TraceRay(const csg::Ray3 &ray, RayQueryCallback cb);

      bool IsPassable(const csg::Point3& at) const;
      bool CanStand(const csg::Point3& at) const { return navgrid_.CanStand(at); }
      void ClipRegion(csg::Region3& r) const { return navgrid_.ClipRegion(r); }

      bool IsStuck(om::EntityPtr object);
      void Unstick(om::EntityPtr object);
      void Unstick(std::vector<csg::Point3> &points);
      csg::Point3 Unstick(const csg::Point3 &pt);

      core::Guard TraceZonesContaining(om::EntityPtr entity);

      TerrainChangeCbId AddCollisionRegionChangeCb(csg::Region3 const* r, TerrainChangeCb cb);
      void RemoveCollisionRegionChangeCb(TerrainChangeCbId id);

      // Path finding helpers
      std::vector<std::pair<csg::Point3, int>> ComputeNeighborMovementCost(const csg::Point3& from) const;
      int EstimateMovementCost(const csg::Point3& src, const csg::Point3& dst) const;
      int EstimateMovementCost(const csg::Point3& src, const std::vector<csg::Point3>& points) const;
      int EstimateMovementCost(const csg::Point3& src, const csg::Region3& dst) const;

   protected:
      bool Intersects(csg::Cube3f bounds, om::RegionCollisionShapePtr rgnCollsionShape) const;   
      bool CanStepUp(csg::Point3& at) const;
      bool CanFallDown(csg::Point3& at) const;

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
      std::map<dm::ObjectId, om::EntityRef>           entities_;
      std::map<dm::ObjectId, om::SensorRef>           sensors_;
      core::Guard                                       guards_;
      NavGrid                                         navgrid_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_OCTTREE_H
 