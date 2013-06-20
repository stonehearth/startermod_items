
#include "pch.h"
#include "floor.h"
#include "mob.h"
#include "render_grid.h"
#include "build_orders.h"
#include "om/stonehearth.h"
#include "om/grid/grid.h"

using namespace ::radiant;
using namespace ::radiant::om;

static const int FloorColor1 = 1;
static const int FloorColor2 = 2;

void Floor::InitializeRecordFields()
{
   GridBuildOrder::InitializeRecordFields();
}

void Floor::Create()
{
   const math3d::color3 palette[] = {
      math3d::color3(0,   0,   0),
      math3d::color3(165, 148, 101), // FloorColor1
      math3d::color3(149, 132,  85), // FloorColor2
   };
   CreateGrid(palette, ARRAY_SIZE(palette));
}

void Floor::Resize(const csg::Point3& size)
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
         LOG(WARNING) << "Updating floor box " << c;
         LOG(WARNING) << "Floor pos:" << GetEntity().GetComponent<Mob>()->GetLocation();
         grid->setVoxel(c, FloorColor1);
      }
   }
}

void Floor::ComputeStandingRegion()
{
   math3d::ipoint3 origin = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();
   
   // Get the entire unreserved region of the floor...
   csg::Region3 missing = GetUnreservedMissingRegion();

   // You must be standing adjacent to block (from any angle).  You can also be
   // 1 unit above the floor (i.e. standing on the floor next to a piece of
   // unbuilt floor).  This dramatically helps the "don't paint myself in a corner"
   // building problem.
   csg::Region3& standing = ModifyStandingRegion();
   standing = Stonehearth::ComputeStandingRegion(missing, 2);

   // The standing region is in world space, so translate.
   standing.Translate(GetWorldOrigin());
}

bool Floor::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
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

void Floor::ConstructBlock(const math3d::ipoint3& block)
{
   csg::Region3& region = ModifyRegion();

   ASSERT(!region.Contains(block));

   region += block;
   ModifyReservedRegion() -= block;

   GridPtr grid = GetGrid();
   grid->GrowBounds(block);
   grid->setVoxel(block, FloorColor1);

   ComputeStandingRegion();
}

void Floor::StartProject(const dm::CloneMapping& mapping)
{
   GridBuildOrder::StartProject(mapping);
   ComputeStandingRegion();
}
