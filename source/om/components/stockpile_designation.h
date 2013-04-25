#ifndef _RADIANT_OM_STOCKPILE_DESIGNATION_H
#define _RADIANT_OM_STOCKPILE_DESIGNATION_H

#include "math3d.h"
#include "om/om.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/set.h"
#include "om/region.h"
#include "namespace.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class StockpileDesignation : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(StockpileDesignation);
   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

   StockpileDesignation();
   ~StockpileDesignation();
   std::string ToString() const;

   const math3d::ibounds3& GetBounds() const { return bounds_; }
   void SetBounds(const math3d::ibounds3& bounds);

   void SetContainer(EntityContainerRef container);

   bool IsFull() const;
   const csg::Region3& GetAvailableRegion() const;
   RegionPtr GetStandingRegion() const { return *standingRgn_; }
   const dm::Set<EntityId>& GetContents() const { return contents_; }

   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved);

   dm::Guard TraceBounds(const char* reason, std::function<void()> fn) { return bounds_.TraceObjectChanges(reason, fn); }
   dm::Guard TraceAvailable(const char* reason, std::function<void()> fn) { return available_.TraceObjectChanges(reason, fn); }
   dm::Guard TraceReserved(const char* reason, std::function<void()> fn) { return reserved_.TraceObjectChanges(reason, fn); }
   dm::Guard TraceContents(const char* reason, std::function<void()> fn) { return contents_.TraceObjectChanges(reason, fn); }

private:
   void InitializeRecordFields() override;
   void AddEntity(dm::ObjectId id, EntityRef e);
   void RemoveEntity(dm::ObjectId id);
   void UpdateStandingRegion();

public:
   dm::GuardSet                    traces_;

   dm::Set<EntityId>               contents_;
   dm::Boxed<math3d::ibounds3>     bounds_;
   dm::Boxed<RegionPtr>            standingRgn_;
   dm::Boxed<csg::Region3>         available_;
   dm::Boxed<csg::Region3>         reserved_;

   mutable std::unordered_map<om::EntityId, math3d::ipoint3>  itemLocations_;
};

static std::ostream& operator<<(std::ostream& os, const StockpileDesignation& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_STOCKPILE_DESIGNATION_H
