#ifndef _RADIANT_PHYSICS_OCTTREE_H
#define _RADIANT_PHYSICS_OCTTREE_H

#include <unordered_map>
#include "namespace.h"
#include "csg/namespace.h"
#include "om/om.h"
#include "dm/dm.h"
#include "nav_grid.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

class OctTree {
   public:
      OctTree(int trace_category);

      void SetRootEntity(om::EntityPtr);
      void Cleanup();

      // good!
      bool CanStandOn(om::EntityPtr entity, const csg::Point3& at) const;
      void RemoveNonStandableRegion(om::EntityPtr e, csg::Region3& r) const;

      // unknown...
      NavGrid& GetNavGrid() { return navgrid_; } // sigh
      NavGrid const& GetNavGrid() const { return navgrid_; } // sigh

      void Update(int now);

      typedef std::function<bool (om::EntityPtr )>   QueryCallback;
      typedef std::function<void (om::EntityPtr, float)> RayQueryCallback;

      void GetActorsIn(const csg::Cube3f &bounds, QueryCallback cb);      
      void TraceRay(const csg::Ray3 &ray, RayQueryCallback cb);

      // Path finding helpers
      std::vector<std::pair<csg::Point3, int>> ComputeNeighborMovementCost(om::EntityPtr entity, const csg::Point3& from) const;
      int EstimateMovementCost(const csg::Point3& src, const csg::Point3& dst) const;
      int EstimateMovementCost(const csg::Point3& src, const std::vector<csg::Point3>& points) const;
      int EstimateMovementCost(const csg::Point3& src, const csg::Region3& dst) const;


      void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);

   protected:
      bool Intersects(csg::Cube3f bounds, om::RegionCollisionShapePtr rgnCollsionShape) const;   
      bool CanStepUp(csg::Point3& at) const;
      bool CanFallDown(csg::Point3& at) const;

      void FilterAllActors(std::function <bool(om::EntityPtr)> filter);
      om::EntityPtr FindFirstActor(om::EntityPtr root, std::function <bool(om::EntityPtr)> filter);

   private:
      void TraceEntity(om::EntityPtr entity);
      void OnComponentAdded(dm::ObjectId id, std::shared_ptr<dm::Object> component);
      void TraceSensor(om::SensorPtr sensor);
      void UpdateSensors();
      bool UpdateSensor(om::SensorPtr sensor);
    
   protected:
      struct EntityMapEntry
      {
         om::EntityRef  entity;
         dm::TracePtr   components_trace;
         dm::TracePtr   children_trace;
         dm::TracePtr   sensor_list_trace;
      };
      std::map<dm::ObjectId, EntityMapEntry>       entities_;
      std::map<dm::ObjectId, om::SensorRef>        sensors_;
      core::Guard                               guards_;
      NavGrid                                   navgrid_;
      int                                       trace_category_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_OCTTREE_H
 