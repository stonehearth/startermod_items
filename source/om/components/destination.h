#ifndef _RADIANT_OM_DESTINATION_H
#define _RADIANT_OM_DESTINATION_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "om/om.h"
#include "om/region.h"
#include "om/all_object_types.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Destination : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Destination, destination);
   static void RegisterLuaType(struct lua_State* L);
   std::string ToString() const;

   void ExtendObject(json::ConstJsonObject const& obj) override;

   RegionPtr const GetRegion() const { return region_; }
   RegionPtr const GetReserved() const { return reserved_; }
   RegionPtr const GetAdjacent() const { return adjacent_; }

   void SetRegion(std::shared_ptr<csg::Region3> region);

private:
   void InitializeRecordFields() override;
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);

public:
   dm::Boxed<RegionPtr>             region_;
   dm::Boxed<RegionPtr>             reserved_;
   dm::Boxed<RegionPtr>             adjacent_;
   dm::GuardSet                     guards_;
   int                              lastUpdated_;
};

static std::ostream& operator<<(std::ostream& os, const Destination& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_DESTINATION_H
