#include "pch.h"
#include "peaked_roof.h"
#include "mob.h"
#include "render_grid.h"
#include "build_orders.h"
#include "om/stonehearth.h"
#include "om/grid/grid.h"

using namespace ::radiant;
using namespace ::radiant::om;

static const int FlatRoofColor1 = 1;
static const int FlatRoofColor2 = 2;

FlatRoof::FlatRoof() :
   GridBuildOrder()
{
   LOG(WARNING) << "creating new peaked roof...";
}

FlatRoof::~FlatRoof()
{
}

void FlatRoof::InitializeRecordFields()
{
   GridBuildOrder::InitializeRecordFields();
}

void FlatRoof::Create()
{
   const math3d::color3 palette[] = {
      math3d::color3(0,   0,   0),
      math3d::color3(165, 148, 101), // FlatRoofColor1
      math3d::color3(149, 132,  85), // FlatRoofColor2
   };
   CreateGrid(palette, ARRAY_SIZE(palette));
}

void FlatRoof::UpdateShape(const csg::Point3& size)
{
   GridBuildOrder::UpdateShape();

   const csg::Region3 oldRegion = GetRegion();
   csg::Region3& rgn = ModifyRegion();

   rgn = csg::Region3(csg::Cube3(csg::Point3(0, 0, 0), size));

   if (!rgn.IsEmpty()) {
      csg::Region3 remove = oldRegion;
      csg::Region3 add = rgn;

      // Remove old blocks...
      GridPtr grid = GetGrid();
      for (const auto &c : remove) {
         grid->setVoxel(c, 0);
      }

      // Fill in the added ones
      grid->setBounds(rgn.GetBounds());
      for (const auto &c : add) {
         LOG(WARNING) << "Updating peaked_roof box " << c;
         LOG(WARNING) << "FlatRoof pos:" << GetEntity().GetComponent<Mob>()->GetLocation();
         grid->setVoxel(c, FlatRoofColor1);
      }
   }
}

void FlatRoof::ComputeStandingRegion()
{
   math3d::ipoint3 origin = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();
   
   // Get the entire unreserved region of the peaked_roof...
   csg::Region3 missing = GetUnreservedMissingRegion();

   // You must be standing adjacent to block (from any angle).  You can also be
   // 1 unit above the peaked_roof (i.e. standing on the peaked_roof next to a piece of
   // unbuilt peaked_roof).  This dramatically helps the "don't paint myself in a corner"
   // building problem.
   csg::Region3& standing = ModifyStandingRegion();
   standing = Stonehearth::ComputeStandingRegion(missing, 2);

   // The standing region is in world space, so translate.
   standing.Translate(GetWorldOrigin());
}

bool FlatRoof::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
{
   const csg::Region3 missing = GetUnreservedMissingRegion();
   const csg::Point3 origin = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();
   const csg::Point3 standing = pt - origin;

   static const csg::Point3 adjacent[] = {
      csg::Point3( 1,  0,  0),
      csg::Point3(-1,  0,  0),
      csg::Point3( 0,  0, -1),
      csg::Point3( 0,  0,  1),
      csg::Point3( 1, -1,  0),
      csg::Point3(-1, -1,  0),
      csg::Point3( 0, -1, -1),
      csg::Point3( 0, -1,  1),
   };

   for (const auto& offset : adjacent) {
      const csg::Point3 block = standing + offset;
      if (missing.Contains(block)) {
         reserved = block;
         ModifyReservedRegion() += reserved;
         ComputeStandingRegion();
         return true;
      }
   }
   return false;
}

void FlatRoof::ConstructBlock(const math3d::ipoint3& block)
{
   csg::Region3& region = ModifyRegion();

   ASSERT(!region.Contains(block));

   region += block;
   ModifyReservedRegion() -= block;

   GridPtr grid = GetGrid();
   grid->GrowBounds(block);
   grid->setVoxel(block, FlatRoofColor1);

   ComputeStandingRegion();
}

void FlatRoof::StartProject(const dm::CloneMapping& mapping)
{
   GridBuildOrder::StartProject(mapping);
   ComputeStandingRegion();
}
