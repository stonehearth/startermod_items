#ifndef _RADIANT_OM_SCAFFOLDING_H
#define _RADIANT_OM_SCAFFOLDING_H

#include "region_build_order.h"
#include "wall.h"

BEGIN_RADIANT_OM_NAMESPACE

class Scaffolding : public RegionBuildOrder
{
public:
   DEFINE_OM_OBJECT_TYPE(Scaffolding);

   void StartProject(WallPtr wall, const dm::CloneMapping& mapping);
   void SetRoof(GridBuildOrderPtr roof);
   csg::Point3 GetNormal() const;
   csg::Point3 GetAbsNormal() const;


   const csg::Region3& GetLadderRegion() const { return ***ladder_; } 
   dm::Guard TraceLadderRegion(const char* reason, std::function<void(const csg::Region3&)> cb) const { return (*ladder_)->Trace(reason, cb); }

public:
   bool ShouldTearDown() override;
   csg::Region3 GetMissingRegion() const override;
   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) override;
   void ConstructBlock(const math3d::ipoint3& block) override;

private:
   void ComputeStandingRegion();
   void InitializeRecordFields() override;
   csg::Region3 GetMissingBuildUpRegion() const; 
   csg::Region3 GetMissingTearDownRegion() const;

private:
   bool                          building_;
   dm::Boxed<RegionPtr>          ladder_;
   dm::Boxed<WallRef>            wall_;
   dm::Boxed<GridBuildOrderRef>  roof_;
   dm::GuardSet                  guards_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_SCAFFOLDING_H
