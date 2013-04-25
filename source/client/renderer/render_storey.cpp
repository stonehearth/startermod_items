#include "pch.h"
#include "render_grid.h"
#include "render_storey.h"
#include "object_model/storey.h"
#include "object_model/structure.h"
#include "object_model/render_grid_object.h"

using namespace ::radiant;
using namespace ::radiant::world;
using namespace ::radiant::client;

RenderStorey::RenderStorey(H3DNode parent, ObjectModel::Storey* storey)
{
   for (auto entry : storey->IterateGrids()) {
      grids_.push_back(new RenderGrid(parent, entry.second));
   }
}

RenderStorey::~RenderStorey()
{
   for (auto grid : grids_) {
      delete grid;
   }
}

void RenderStorey::PrepareToRender(int now, float distance)
{
   for (auto grid : grids_) {
      grid->PrepareToRender(now, distance);
   }
}

void RenderStorey::AddToSelection(om::Selection& sel, const math3d::ray3& ray, const math3d::point3& intersection, const math3d::point3& normal)
{
}
