#ifndef _RADIANT_OM_GRID_BUILD_ORDER_H
#define _RADIANT_OM_GRID_BUILD_ORDER_H

#include "region_build_order.h"

BEGIN_RADIANT_OM_NAMESPACE

class GridBuildOrder : public RegionBuildOrder
{
public:
   void StartProject(const dm::CloneMapping& mapping) override;

protected:
   void InitializeRecordFields() override;

protected:
   GridPtr GetGrid() { return *grid_; }
   void CreateGrid(const math3d::color3* palette, int c);

private:
   dm::Boxed<GridPtr>            grid_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_GRID_BUILD_ORDER_H
