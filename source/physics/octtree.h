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

      typedef std::function<bool (om::EntityPtr )>   QueryCallback;
      typedef std::function<void (om::EntityPtr, float)> RayQueryCallback;

      void GetActorsIn(const csg::Cube3f &bounds, QueryCallback cb);      
      void TraceRay(const csg::Ray3 &ray, RayQueryCallback cb);

      // Path finding helpers
      typedef std::vector<std::pair<csg::Point3, float>> MovementCostVector;

      MovementCostVector ComputeNeighborMovementCost(om::EntityPtr entity, const csg::Point3& from) const;
      float GetMovementCost(const csg::Point3& src, const csg::Point3& dst) const;

      template <class T>
      bool CanStandOnOneOf(om::EntityPtr const& entity, std::vector<csg::Point<T,3>> const& points, csg::Point<T,3>& standablePoint) const;

      bool ValidMove(om::EntityPtr const& entity, bool const reversible, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const;

      void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);

   protected:
      bool Intersects(csg::Cube3f bounds, om::RegionCollisionShapePtr rgnCollsionShape) const;   
      void FilterAllActors(std::function <bool(om::EntityPtr)> filter);
      om::EntityPtr FindFirstActor(om::EntityPtr root, std::function <bool(om::EntityPtr)> filter);
      bool ValidDiagonalMove(om::EntityPtr const& entity, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const;
      bool ValidElevationChange(om::EntityPtr const& entity, bool const reversible, csg::Point3 const& fromLocation, csg::Point3 const& toLocation) const;

   private:
      void TraceEntity(om::EntityPtr entity);
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
      core::Guard                               guards_;
      mutable NavGrid                           navgrid_;
      int                                       trace_category_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_OCTTREE_H
 