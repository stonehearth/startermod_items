#ifndef _RADIANT_OM_FLOOR_H
#define _RADIANT_OM_FLOOR_H

#include "om/om.h"
#include "grid_build_order.h"

BEGIN_RADIANT_OM_NAMESPACE

class Floor : public GridBuildOrder
{
public:
   DEFINE_OM_OBJECT_TYPE(Floor, floor);

   void Create();

public: // BuildOrder overrides
   void Resize(const csg::Point3& size);
   void ConstructBlock(const math3d::ipoint3& block) override;
   bool ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved) override;
   void StartProject(const dm::CloneMapping& mapping) override;

private:
   void InitializeRecordFields() override;
   void ComputeStandingRegion();

private:
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_FLOOR_H
