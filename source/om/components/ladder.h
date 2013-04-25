#ifndef _RADIANT_OM_LADDER_H
#define _RADIANT_OM_LADDER_H

#include "build_order.h"
#include "wall.h"

BEGIN_RADIANT_OM_NAMESPACE

class Ladder : public BuildOrder
{
public:
   DEFINE_OM_OBJECT_TYPE(Ladder);

   Ladder();
   virtual ~Ladder();

   void StartProject();
   void SetSupportStructure(WallPtr wall);

public:
   csg::Region3 GetMissingRegion() const override;
   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) override;
   void ConstructBlock(const math3d::ipoint3& block) override;

private:
   void ComputeStandingRegion();
   void InitializeRecordFields() override;

private:
   dm::Boxed<WallRef>      wall_;
   dm::GuardSet            guards_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_LADDER_H
