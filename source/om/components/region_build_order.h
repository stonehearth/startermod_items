#ifndef _RADIANT_OM_REGION_BUILD_ORDER_H
#define _RADIANT_OM_REGION_BUILD_ORDER_H

#include "build_order.h"

BEGIN_RADIANT_OM_NAMESPACE

class RegionBuildOrder : public BuildOrder
{
public: // BuildOrder overrides
   bool NeedsMoreWork() override;
   const RegionPtr GetConstructionRegion() const override;
   void StartProject(const dm::CloneMapping& mapping) override;

   const csg::Region3& GetRegion() const override { return ***region_; } 
   const csg::Region3& GetReservedRegion() const override { return *reserved_; } 
   dm::Guard TraceRegion(const char* reason, std::function<void(const csg::Region3&)> cb) const override { return (*region_)->Trace(reason, cb); }
   dm::Guard TraceReserved(const char* reason, std::function<void(const csg::Region3&)> cb) const override { return reserved_.Trace(reason, cb); }

   void CompleteTo(int percent) override;

public:
   const RegionPtr GetRegionPtr() const { return *region_; } 
   
public:
   virtual csg::Region3 GetMissingRegion() const;

protected:
   void CreateRegion();
   csg::Region3& ModifyRegion() { return (*region_)->Modify(); }
   csg::Region3& ModifyReservedRegion() { return reserved_.Modify(); }
   csg::Region3& ModifyStandingRegion() { return (*standingRegion_)->Modify(); }
   csg::Region3 GetUnreservedMissingRegion() const;
   RegionBuildOrderPtr GetBlueprint() const { return (*blueprint_).lock(); }

protected:
   void InitializeRecordFields() override;

private:
   dm::Boxed<RegionPtr>             region_;
   dm::Boxed<RegionPtr>             standingRegion_;
   dm::Boxed<RegionBuildOrderRef>   blueprint_;
   Region                           reserved_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_BUILD_ORDER_H
