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
      OctTree(dm::TraceCategories trace_category);
      virtual ~OctTree();

      void SetRootEntity(om::EntityPtr);

      void EnableSensorTraces(bool enabled);

      // unknown...
      NavGrid& GetNavGrid() { return navgrid_; } // sigh
      NavGrid const& GetNavGrid() const { return navgrid_; } // sigh

      typedef std::function<bool (om::EntityPtr )>   QueryCallback;
      typedef std::function<void (om::EntityPtr, float)> RayQueryCallback;

      // Path finding helpers
      typedef std::function<void(csg::Point3 const&, float)> MovementCostCb;

      void ComputeNeighborMovementCost(om::EntityPtr entity, const csg::Point3& from, MovementCostCb const& cb) const;
      float GetAdjacentMovementCost(const csg::Point3& src, const csg::Point3& dst) const;
      float GetSquaredAdjacentMovementCost(const csg::Point3& src, const csg::Point3& dst) const;

      float GetDistanceCost(const csg::Point3& src, const csg::Point3& dst) const;
      float GetSquaredDistanceCost(const csg::Point3& src, const csg::Point3& dst) const;

      bool ValidMove(om::EntityPtr const& entity, bool const reversible, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const;
      bool ValidMove(NavGrid::IsStandableQuery const& q, bool const reversible, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const;

   protected:
      bool Intersects(csg::Cube3f bounds, om::RegionCollisionShapePtr rgnCollsionShape) const;   
      bool ValidDiagonalMove(NavGrid::IsStandableQuery const& q, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const;
      bool ValidElevationChange(om::EntityPtr const& entity, bool const reversible, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const;

   private:
      void TraceEntity(om::EntityPtr entity);
      void UnTraceEntity(dm::ObjectId id);
      void OnComponentAdded(dm::ObjectId id, om::ComponentPtr component);
    
   protected:
      struct EntityMapEntry
      {
         om::EntityRef  entity;
         dm::TracePtr   components_trace;
         dm::TracePtr   children_trace;
         dm::TracePtr   sensor_list_trace;
      };
      std::map<dm::ObjectId, EntityMapEntry>    entities_;
      std::map<dm::ObjectId, std::pair<SensorTrackerPtr, dm::TracePtr>>  sensor_trackers_;
      mutable NavGrid                           navgrid_;
      bool                                      enable_sensor_traces_;
      dm::TraceCategories                       trace_category_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_OCTTREE_H
 