#ifndef _RADIANT_OM_RESOURCE_NODE_H
#define _RADIANT_OM_RESOURCE_NODE_H

#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class ResourceNode : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(ResourceNode);

public:
   std::vector<math3d::ipoint3> GetHarvestStandingLocations() const;

   std::string GetResource() const;
   void SetResource(std::string res);

   int GetDurability() const;
   void SetDurability(int durability);

private:
   NO_COPY_CONSTRUCTOR(ResourceNode)

   void InitializeRecordFields() override;

private:
   dm::Boxed<std::string>     resource_;
   dm::Boxed<int>             durability_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_RESOURCE_NODE_H
