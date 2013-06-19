#include "pch.h"
#include "post.h"
#include "mob.h"
#include "render_grid.h"
#include "build_orders.h"
#include "om/grid/grid.h"

using namespace ::radiant;
using namespace ::radiant::om;

static const int PostColor1 = 1;

void Post::InitializeRecordFields()
{
   GridBuildOrder::InitializeRecordFields();
   AddRecordField("height", height_);
}

void Post::Create(int height)
{
   const math3d::color3 palette[] = {
      math3d::color3(0,   0,   0),
#if 0
      math3d::color3(64,  55,  38),     // PostColor1
#else
      math3d::color3(51,  43,  31),     // PostColor1
#endif
   };
   CreateGrid(palette, ARRAY_SIZE(palette));

   height_ = height;

   // Modify our region to contain the whole height and fill in the grid
   ModifyRegion() = csg::Region3(csg::Cube3(csg::Point3(0, 0, 0), csg::Point3(1, height, 1)));

   GridPtr grid = GetGrid();
   csg::Cube3 bounds = GetRegion().GetBounds();
   grid->setBounds(bounds);
   grid->setVoxel(bounds, PostColor1);
}

void Post::StartProject(const dm::CloneMapping& mapping)
{
   GridBuildOrder::StartProject(mapping);
   ComputeStandingRegion();
   height_ = 0;
}

void Post::ComputeStandingRegion()
{
   csg::Region3& result = ModifyStandingRegion();

   math3d::ipoint3 location = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();

   result += location + math3d::ipoint3( 1,  0,  0);
   result += location + math3d::ipoint3(-1,  0,  0);
   result += location + math3d::ipoint3( 0,  0,  1);
   result += location + math3d::ipoint3( 0,  0, -1);
}

bool Post::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
{
   // reserve the bottom block...
   csg::Region3 region = GetUnreservedMissingRegion();
   if (region.IsEmpty()) {
      return false;
   }
   reserved = region.begin()->GetMin();
   ModifyReservedRegion() += reserved;
   return true;
}
 
void Post::ConstructBlock(const math3d::ipoint3& block)
{
   auto blueprint = GetBlueprint();
   if (blueprint) {
      // Doesn't really matter which block, since the workers could come in a
      // different order than the reservations when out

      // knock off the bottom reservation...
      csg::Region3& reserved = ModifyReservedRegion();
      reserved -= reserved.begin()->GetMin();

      // grow the post by one.
      csg::Region3 missing = blueprint->GetRegion() - GetRegion();
      if (!missing.IsEmpty()) {
         auto pt = missing.begin()->GetMin();
         ModifyRegion() += pt;

         GridPtr grid = GetGrid();
         grid->GrowBounds(pt);
         grid->setVoxel(pt, PostColor1);
      }
   }
}
