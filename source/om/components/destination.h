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

   Region3BoxedPtrBoxed const& GetRegion() const { return region_; }
   Region3BoxedPtrBoxed const& GetReserved() const { return reserved_; }
   Region3BoxedPtrBoxed const& GetAdjacent() const { return adjacent_; }

   void SetRegion(Region3BoxedPtr r);
   void SetReserved(Region3BoxedPtr r);
   void SetAdjacent(Region3BoxedPtr r);

   bool GetAutoUpdateAdjacent() const { return *auto_update_adjacent_; }
   Destination& SetAutoUpdateAdjacent(bool value);

private:
   void InitializeRecordFields() override;
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);

public:
   dm::Boxed<bool>                  auto_update_adjacent_;
   Region3BoxedPtrBoxed       region_;
   Region3BoxedPtrBoxed       reserved_;
   Region3BoxedPtrBoxed       adjacent_;
   DeepRegionGuardPtr              region_guard_;
   DeepRegionGuardPtr              reserved_guard_;
   core::Guard                        update_adjacent_guard_;
   int                              lastUpdated_;
   int                              lastChanged_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_DESTINATION_H
