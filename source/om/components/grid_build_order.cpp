#include "pch.h"
#include "om/grid/grid.h"
#include "grid_build_order.h"
#include "render_grid.h"
#include "region_collision_shape.h"

using namespace ::radiant;
using namespace ::radiant::om;

void GridBuildOrder::InitializeRecordFields()
{
   RegionBuildOrder::InitializeRecordFields();
   AddRecordField("grid",        grid_);
}

void GridBuildOrder::StartProject(const dm::CloneMapping& mapping)
{
   RegionBuildOrder::StartProject(mapping); 

   GetGrid()->Clear();
}

void GridBuildOrder::CreateGrid(const math3d::color3* palette, int c) 
{
   RegionBuildOrder::CreateRegion();

   grid_ = GetStore().AllocObject<Grid>();
   (*grid_)->SetPalette(palette, c);

   RenderGridPtr render_grid = GetEntity().AddComponent<RenderGrid>();
   render_grid->SetGrid(*grid_);
}
