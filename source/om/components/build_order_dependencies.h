#ifndef _RADIANT_OM_BUILD_ORDER_DEPENDENCIES_H
#define _RADIANT_OM_BUILD_ORDER_DEPENDENCIES_H

#include "math3d.h"
#include "om/om.h"
#include "dm/set.h"
#include "dm/record.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class BuildOrderDependencies : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(BuildOrderDependencies, build_order_dependencies);

   typedef dm::Set<EntityRef> Dependencies;

   void AddDependency(EntityRef dep);
   const Dependencies& GetDependencies() const;

protected:
   void InitializeRecordFields() override;

private:
   Dependencies               dependencies_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_BUILD_ORDER_DEPENDENCIES_H
