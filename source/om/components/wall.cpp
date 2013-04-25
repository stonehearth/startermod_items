#include "pch.h"
#include "wall.h"
#include "mob.h"
#include "post.h"
#include "render_grid.h"
#include "build_orders.h"
#include "om/grid/grid.h"
#include "build_order_dependencies.h"
#include "entity_container.h"
#include "component_helpers.h"
#include "portal.h"
#include "fixture.h"
using namespace ::radiant;
using namespace ::radiant::om;

static const int WallColor1 = 1;
static const int WallColor2 = 2;

Wall::Wall() :
   GridBuildOrder()
{
   LOG(WARNING) << "creating new wall...";
}

Wall::~Wall()
{
}

void Wall::InitializeRecordFields()
{
   GridBuildOrder::InitializeRecordFields();
   AddRecordField("post1", post1_);
   AddRecordField("post2", post2_);
   AddRecordField("size",  size_);
   AddRecordField("normal", normal_);
   AddRecordField("fixtures", fixtures_);
}

void Wall::Create(PostPtr post1, PostPtr post2, const csg::Point3& normal)
{
   const math3d::color3 palette[] = {
      math3d::color3(0,   0,   0),
#if 0
      math3d::color3(165, 159, 139), // Main
      math3d::color3(148, 143, 126), // Detail
#else
      math3d::color3(192, 187, 168), // Main
      math3d::color3(165, 159, 139), // Detail
#endif
   };

   CreateGrid(palette, ARRAY_SIZE(palette));
   auto dep = GetEntity().AddComponent<BuildOrderDependencies>();
   dep->AddDependency(post1->GetEntityRef());
   dep->AddDependency(post2->GetEntityRef());

   post1_ = post1;
   post2_ = post2;
   normal_ = normal;
}

void Wall::Resize(const csg::Region3& roofRegion)
{
   GridBuildOrder::UpdateShape();
   csg::Point3 size(csg::Point3(0, 0, 0));

   auto post1 = (*post1_).lock();
   auto post2 = (*post2_).lock();

   ASSERT(post1 && post2);

   csg::Point3 p0 = post1->GetEntity().GetComponent<Mob>()->GetGridLocation();
   csg::Point3 p1 = post2->GetEntity().GetComponent<Mob>()->GetGridLocation();

   int tangent, normal;
   if (p0[0] != p1[0]) {
      tangent = 0;
      normal = 2;
   } else {
      tangent = 2;
      normal = 0;
   }
   ASSERT(p0[normal] == p1[normal]);

   // The walls start inside the posts..., so the total width is going to
   // be 1 smaller
   int width = p1[tangent] - p0[tangent] - 1;
   if (width > 0) {
      size[tangent] = width;
      size[1] = std::min(post1->GetHeight(), post2->GetHeight());
      size[normal] = 1;
   }

   // Modify our region to contain the whole height
   size_ = size;

   const csg::Region3 oldRegion = GetRegion();
   csg::Region3& rgn = ModifyRegion();
   csg::Cube3 bounds(csg::Point3(0, 0, 0), *size_);
   rgn = csg::Region3(bounds);

   // Move next to the post
   csg::Point3 pos = post1->GetEntity().GetComponent<Mob>()->GetGridLocation();
   pos[tangent] += 1;
   GetEntity().GetComponent<Mob>()->MoveToGridAligned(pos);

   // Add in the parts of the region needed to support the roof...
   csg::Region3 localRoofRegion = roofRegion;
   localRoofRegion.Translate(-GetEntityWorldGridLocation(GetEntity()));

   // Cut out the parts of the roof that we support...
   const csg::Point3 min = bounds.GetMin();
   const csg::Point3 max = bounds.GetMax();
   localRoofRegion &= csg::Cube3(csg::Point3(min.x, min.y, min.z),
                                 csg::Point3(max.x, INT_MAX, max.z));

   // Add in slices...
   int top = bounds.GetMax().y;
   for (const csg::Cube3& c : localRoofRegion) {
      const csg::Point3 min = c.GetMin();
      const csg::Point3 max = c.GetMax();
      ASSERT(min.y >= top);
      if (max.y > (top + 1)) {
         rgn += csg::Cube3(csg::Point3(min.x, top, min.z),
                           csg::Point3(max.x, max.y - 1, max.z));
      }
   }
   CommitRegion(oldRegion);
   StencilOutPortals();
}

void Wall::CommitRegion(const csg::Region3& oldRegion)
{
   // Compute add and remove regions
   //csg::Region3 remove = oldRegion - rgn;
   //csg::Region3 add = rgn - oldRegion;
   const csg::Region3& rgn = GetRegion();
   csg::Region3 remove = oldRegion - rgn;
   csg::Region3 add = rgn - oldRegion;

   LOG(WARNING) << "UPDATING WALL " << GetObjectId();

   // Remove old blocks...
   GridPtr grid = GetGrid();
   for (const csg::Cube3 &c : remove) {
      grid->setVoxel(c, 0);
   }

   // Fill in the added ones
   if (!rgn.IsEmpty()) {
      grid->setBounds(rgn.GetBounds());
      for (const csg::Cube3 &c : add) {
         for (const csg::Point3 pt : c) {
            grid->setVoxel(pt, GetColor(pt));
         }
      }
   }
}

void Wall::StencilOutPortals()
{
   const csg::Region3 oldRegion = GetRegion();
   csg::Region3& rgn = ModifyRegion();

   // This will erase all the existing portals except for the ones which
   // overlap the peak supporting a PeakedRoof.  Good enough for now!
   rgn += csg::Cube3(csg::Point3(0, 0, 0), *size_);

   int normal, tangent;
   if ((*normal_).x) {
      normal = 0, tangent = 2;
   } else {
      normal = 2, tangent = 0;
   }

   // Remove the portals...
   for (const auto& entity : fixtures_) {
      auto portal = entity->GetComponent<Portal>();
      if (portal) {
         const auto& origin = entity->GetComponent<Mob>()->GetGridLocation();
         for (const csg::Rect2& rc : portal->GetRegion()) {
            csg::Point3 min, max;
            const csg::Point2& rcMin = rc.GetMin();
            const csg::Point2& rcMax = rc.GetMax();
            min[normal] = INT_MIN;
            max[normal] = INT_MAX;
            min[tangent] = rcMin.x;
            max[tangent] = rcMax.x;
            min[1] = rcMin.y;
            max[1] = rcMax.y;
            rgn -= csg::Cube3(min, max) + origin;
         }
      }
   }
   CommitRegion(oldRegion);
}

void Wall::ComputeStandingRegion()
{
   math3d::ipoint3 origin = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();
   
   // Get the entire unreserved region of the wall...
   csg::Region3& result = ModifyStandingRegion();
   result = GetUnreservedMissingRegion();

   // Step away from the wall (only on the outside) and move to world coordinates
   result.Translate(origin + *normal_);
}

bool Wall::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
{
   const csg::Region3 missing = GetUnreservedMissingRegion();
   const csg::Point3 origin = GetEntity().GetComponent<Mob>()->GetWorldGridLocation();
   const csg::Point3 standing = pt - origin;

   static const csg::Point3 adjacent[] = {
      csg::Point3( 1,  0,  0),
      csg::Point3(-1,  0,  0),
      csg::Point3( 0,  0, -1),
      csg::Point3( 0,  0,  1),
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

void Wall::ConstructBlock(const math3d::ipoint3& block)
{
   csg::Region3& region = ModifyRegion();

   ASSERT(!region.Contains(block));

   region += block;
   ModifyReservedRegion() -= block;

   GridPtr grid = GetGrid();
   grid->GrowBounds(block);
   grid->setVoxel(block, GetColor(block));

   ComputeStandingRegion();
}

void Wall::StartProject(const dm::CloneMapping& mapping)
{
   GridBuildOrder::StartProject(mapping);
   ComputeStandingRegion();
   for (EntityPtr e : fixtures_) {
      auto fixture = e->GetComponent<Fixture>();
      if (fixture) {
         fixture->StartProject(mapping);
      }
   }
}

void Wall::AddFixture(om::EntityRef f)
{
   auto fixture = f.lock();
   if (fixture) {
      // Add the fixture to our container so it gets rendered properly
      auto container = GetEntity().AddComponent<EntityContainer>();
      container->AddChild(fixture);
      fixtures_.Insert(fixture);
      StencilOutPortals();
   }
}

int Wall::GetColor(const csg::Point3& loc)
{
#if 0
   auto rand = [this]() {
      seed_ = seed_* 1103515245 + 12345;
      return (UINT32)(seed_ >> 16) & RAND_MAX;
   };
#endif

   if (loc.y == 0) {
      return WallColor2;
   }
   if (loc.y == 1) {
      return ((loc.x + loc.z) & 1) ? WallColor2 : WallColor1;
   }
   return WallColor1;
}
