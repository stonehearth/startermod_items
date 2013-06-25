#ifndef _RADIANT_OM_PEAKED_ROOF_H
#define _RADIANT_OM_PEAKED_ROOF_H

#include "om/om.h"
#include "grid_build_order.h"

BEGIN_RADIANT_OM_NAMESPACE

class PeakedRoof : public GridBuildOrder
{
public:
   DEFINE_OM_OBJECT_TYPE(PeakedRoof, peaked_roof);

   void Create();

public: // BuildOrder overrides
   csg::Region3 GetMissingRegion() const override;
   void UpdateShape(const csg::Point3& size);
   void ConstructBlock(const math3d::ipoint3& block) override;
   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) override;
   void StartProject(const dm::CloneMapping& mapping) override;

private:
   void InitializeRecordFields() override;
   void ComputeStandingRegion();
   int GetColor(const csg::Point3& loc, const csg::Point3& size) const;

private:
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_PEAKED_ROOF_H
