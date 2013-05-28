#ifndef _RADIANT_OM_BUILD_ORDERS_H
#define _RADIANT_OM_BUILD_ORDERS_H

#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"
#include "build_order.h"

BEGIN_RADIANT_OM_NAMESPACE

class BuildOrders : public Component,
                    public std::enable_shared_from_this<BuildOrders> {
public:
   DEFINE_OM_OBJECT_TYPE(BuildOrders);

   typedef dm::Set<EntityRef> ProjectList;
   typedef dm::Set<EntityRef> BlueprintList;
   typedef dm::Set<EntityRef> BuildOrderList;

   const ProjectList& GetProjects() { return projects_; }
   const BlueprintList& GetBlueprints() { return blueprints_; }
   const BuildOrderList& GetInProgress() { return inprogress_; }

public:
   void AddBlueprint(EntityRef blueprint);
   bool StartProject(EntityRef blueprint, EntityRef project);

private:
   NO_COPY_CONSTRUCTOR(BuildOrders)

   void InitializeRecordFields() override;
   //void StartBuildProject(BuildOrderPtr blueprint, BuildOrderPtr project); 

private:
   BlueprintList           blueprints_;
   BlueprintList           runningProjects_;
   ProjectList             projects_;
   BuildOrderList          inprogress_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_BUILD_ORDERS_H
