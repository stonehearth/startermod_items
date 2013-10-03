#ifndef _RADIANT_OM_DESTINATION_H
#define _RADIANT_OM_DESTINATION_H

#include "dm/boxed.h"
#include "dm/record.h"
#include "om/om.h"
#include "om/region.h"
#include "om/object_enums.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Destination : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Destination, destination);

   void ExtendObject(json::ConstJsonObject const& obj) override;

   dm::Boxed<BoxedRegion3Ptr> const& GetRegion() const { return region_; }
   dm::Boxed<BoxedRegion3Ptr> const& GetReserved() const { return reserved_; }
   dm::Boxed<BoxedRegion3Ptr> const& GetAdjacent() { return adjacent_; }

   void SetRegion(BoxedRegion3Ptr r);
   void SetReserved(BoxedRegion3Ptr r);
   void SetAdjacent(BoxedRegion3Ptr r);

   void SetAutoUpdateAdjacent(bool value);

private:
   void InitializeRecordFields() override;
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);

public:
   dm::Boxed<bool>                  auto_update_adjacent_;
   dm::Boxed<BoxedRegion3Ptr>       region_;
   dm::Boxed<BoxedRegion3Ptr>       reserved_;
   dm::Boxed<BoxedRegion3Ptr>       adjacent_;
   BoxedRegionGuardPtr              region_guard_;
   BoxedRegionGuardPtr              reserved_guard_;
   dm::Guard                        update_adjacent_guard_;
   int                              lastUpdated_;
   int                              lastChanged_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_DESTINATION_H
