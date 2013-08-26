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

   BoxedRegion3Ptr GetRegion() const { return region_; }
   void SetRegion(BoxedRegion3Ptr r) { region_ = r; }

   BoxedRegion3Ptr GetReserved() const;
   BoxedRegion3Ptr GetAdjacent();

private:
   void InitializeRecordFields() override;
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);

public:
   dm::Boxed<BoxedRegion3Ptr>       region_;
   dm::Boxed<BoxedRegion3Ptr>       reserved_;
   dm::Boxed<BoxedRegion3Ptr>       adjacent_;
   dm::Guard                        guards_;
   int                              lastUpdated_;
   int                              lastChanged_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_DESTINATION_H
