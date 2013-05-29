#include "pch.h"
#include "peaked_roof.h"
#include "mob.h"
#include "render_grid.h"
#include "build_orders.h"
#include "om/stonehearth.h"
#include "om/grid/grid.h"

using namespace ::radiant;
using namespace ::radiant::om;

static const int RoofColorNeutral = 1;
static const int RoofColorBright = 2;
static const int RoofColorDark = 3;

void PeakedRoof::InitializeRecordFields()
{
   GridBuildOrder::InitializeRecordFields();
}

void PeakedRoof::Create()
{
   const math3d::color3 palette[] = {
      math3d::color3(0,   0,   0),
#if 0
      math3d::color3(149, 104,  55), // Base
      math3d::color3(176, 129,  78), // Edges
      math3d::color3(116,  84,  50), // Beams
#else
      math3d::color3( 65,  43,  31), // Base
      math3d::color3( 90,  61,  45), // Edges
      math3d::color3( 44,  33,  24), // Beams
#endif
   };
   CreateGrid(palette, ARRAY_SIZE(palette));
}

void PeakedRoof::UpdateShape(const csg::Point3& size)
{
   GridBuildOrder::UpdateShape();

   const csg::Region3 oldRegion = GetRegion();
   csg::Region3& rgn = ModifyRegion();
   rgn.Clear();


   if (size.x > 2 && size.z > 2) {
      int normal = (size.x > size.z) ? 2 : 0;

      //csg::Cube3 box(csg::Point3(-1, 0, -1), size + csg::Point3(1, 0, 1));
      csg::Cube3 box(csg::Point3(0, 0, 0), size);
      while (!box.IsEmpty()) {
         csg::Point3 min = box.GetMin();
         csg::Point3 max = box.GetMax();

         if (normal == 0) {
            rgn += csg::Cube3(csg::Point3(min.x, min.y, min.z), csg::Point3(min.x + 1, max.y, max.z)); // left
            rgn += csg::Cube3(csg::Point3(max.x - 1, min.y, min.z), csg::Point3(max.x, max.y, max.z)); // right
         } else {
            rgn += csg::Cube3(csg::Point3(min.x, min.y, min.z), csg::Point3(max.x, max.y, min.z + 1)); // top
            rgn += csg::Cube3(csg::Point3(min.x, min.y, max.z - 1), csg::Point3(max.x, max.y, max.z)); // bottom
         }
         min.y++;
         max.y++;
         min[normal]++;
         max[normal] = std::max(min[normal], max[normal] - 1);
         box = csg::Cube3(min, max);
      }
   }

   csg::Region3 remove = oldRegion;
   csg::Region3 add = rgn;

   // Remove old blocks...
   GridPtr grid = GetGrid();
   for (const auto &c : remove) {
      grid->setVoxel(c, 0);
   }

   // Fill in the added ones
   if (!rgn.IsEmpty()) {
      csg::Cube3 bounds = rgn.GetBounds();
      csg::Point3 size = bounds.GetSize();
      grid->setBounds(bounds);
      for (const csg::Cube3 &c : add) {
         for (const csg::Point3 pt : c) {
            grid->setVoxel(pt, GetColor(pt, size));
         }
      }
   }
}

void PeakedRoof::ComputeStandingRegion()
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

bool PeakedRoof::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
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

void PeakedRoof::ConstructBlock(const math3d::ipoint3& block)
{
   csg::Region3& region = ModifyRegion();

   ASSERT(!region.Contains(block));

   region += block;
   ModifyReservedRegion() -= block;

   GridPtr grid = GetGrid();
   grid->GrowBounds(block);
   grid->setVoxel(block, GetColor(block, region.GetBounds().GetSize()));

   ComputeStandingRegion();
}

void PeakedRoof::StartProject(const dm::CloneMapping& mapping)
{
   GridBuildOrder::StartProject(mapping);
   ComputeStandingRegion();
}

csg::Region3 PeakedRoof::GetMissingRegion() const
{
   csg::Region3 missing = GridBuildOrder::GetMissingRegion();

   // We build peaked roofs from the bottom up, so strip out all
   // the crap above the bottom row.
   if (!missing.IsEmpty()) {
      int bottom = missing.GetBounds().GetMin().y;
      missing -= csg::Cube3(csg::Point3(INT_MIN, bottom + 1, INT_MIN),
                            csg::Point3(INT_MAX, INT_MAX, INT_MAX));
   }
   return missing;
}


int PeakedRoof::GetColor(const csg::Point3& loc, const csg::Point3& size) const
{
#if 0
   auto rand = [this]() {
      seed_ = seed_* 1103515245 + 12345;
      return (UINT32)(seed_ >> 16) & RAND_MAX;
   };
#endif
   int coord = (size.x > size.z) ? 0 : 2;
   if (loc[coord] % 3 == 1) {
      return RoofColorDark;
   }
   if (loc.x == 0 || loc.x == size.x - 1 ||
       loc.z == 0 || loc.z == size.z - 1) {
      return RoofColorBright;
   }
   return RoofColorNeutral;
}
