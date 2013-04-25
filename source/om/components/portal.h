#ifndef _RADIANT_OM_PORTAL_H
#define _RADIANT_OM_PORTAL_H

#include "component.h"
#include "om/region.h"
#include "resources/region2d.h"

BEGIN_RADIANT_OM_NAMESPACE

class Portal : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Portal);

   const csg::Region2& GetRegion() const { return *region_; }
   void SetPortal(const resources::Region2d& region);

private:
   void InitializeRecordFields() override;

public:
   dm::Boxed<csg::Region2>    region_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_PORTAL_H
