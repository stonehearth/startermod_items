#include "pch.h"
#include "renderer.h"
#include "render_grid.h"
#include "pipeline.h"
#include "om/grid/grid.h"
#include "om/selection.h"
#include "om/components/render_grid.h"
#include "om/grid/grid.h"
#include "Horde3DUtils.h"
#include "render_entity.h"
#include "pipeline.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderGrid::RenderGrid(const RenderEntity& entity, om::RenderGridPtr renderGrid) :
   entity_(entity),
   renderGrid_(renderGrid)
{
   auto grid = renderGrid->GetGrid();

   H3DNode parent = entity.GetNode();
   node_ = h3dAddGroupNode(parent, "grid");
   h3dSetNodeFlags(node_, h3dGetNodeFlags(parent), true);

   tracer_ += Renderer::GetInstance().TraceFrameStart(std::bind(&RenderGrid::FlushDirtyTiles, this));
   tracer_ += Renderer::GetInstance().TraceSelected(node_, std::bind(&RenderGrid::OnSelected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

   auto added = std::bind(&RenderGrid::AddDirtyTile, this, std::placeholders::_1, std::placeholders::_2);
   auto removed = std::bind(&RenderGrid::RemoveTile, this, std::placeholders::_1);

   if (grid) {
      tracer_ += grid->GetTiles().TraceMapChanges("render grid tiles", added, removed);
      UpdateAllTiles();
   }
}

RenderGrid::~RenderGrid()
{
   DestroyRenderNodes();
   if (node_) {
      h3dRemoveNode(node_);
      node_ = 0;
   }
}

om::GridPtr RenderGrid::GetGrid() const
{
   om::GridPtr grid;
   om::RenderGridPtr renderGrid = renderGrid_.lock();
   if (renderGrid) {
      grid = renderGrid->GetGrid();
   }
   return grid;
}

void RenderGrid::UpdateAllTiles()
{
   const om::GridPtr grid = GetGrid();
   if (grid) {
      DestroyRenderNodes();
      for (const auto& entry : grid->GetTiles()) {
         stdutil::UniqueInsert(_dirtyTiles, entry.first);
      }
   }
}

void RenderGrid::AddDirtyTile(const om::TileAddress& addr, const om::GridTilePtr tile)
{
   stdutil::UniqueInsert(_dirtyTiles, addr);
}

void RenderGrid::RemoveTile(const om::TileAddress& addr)
{
   stdutil::UniqueInsert(_dirtyTiles, addr);
}

void RenderGrid::FlushDirtyTiles()
{
   const om::GridPtr grid = GetGrid();
   if (grid) {
      for (const auto& addr : _dirtyTiles) {
         const auto& tiles = grid->GetTiles().GetContents();

         auto i = tiles.find(addr);
         if (i == tiles.end()) {
            LOG(WARNING) << "REMOVING TILE @ " << addr << ".";
            _tiles.erase(addr);
            continue;
         }

         om::GridTilePtr tile = i->second;
         H3DNode node = Pipeline::GetInstance().GetTileEntity(grid, tile, node_);
         auto entry = std::make_shared<TileEntry>(node);
         entry->traces += tile->TraceObjectChanges("render grid tile changed", [=]() {
            stdutil::UniqueInsert(_dirtyTiles, addr);
         });

         // xxxx - This is more expensive than it needs to be.  If the actual tile object
         // in the grid is the same as the one we used last time for this addr, there's
         // no need to create a new trace
         _tiles[addr] = entry;
      };
      _dirtyTiles.clear();
   }
}

void RenderGrid::DestroyRenderNodes()
{
   _tiles.clear();
}

#if 0
#if 1
void RenderGrid::AddToSelection(om::Selection& sel, const math3d::ray3& ray, const math3d::point3& point, const math3d::point3& normal)
{
   const om::GridPtr grid = renderGrid_->GetGrid();

   math3d::ipoint3 brick;

   // xxx: massive violation of dry principles
   brick.x = (int)(point.x + 0.5f + (point.x >= 0 ? k_epsilon : (-1.0f+k_epsilon)));
   brick.y = (int)(point.y + 0.5f + (point.y >= 0 ? k_epsilon : (-1.0f+k_epsilon)));
   brick.z = (int)(point.z + 0.5f + (point.z >= 0 ? k_epsilon : (-1.0f+k_epsilon)));

#if 0
   char buf[1024];
   sprintf(buf, "%d, %d, %d  - %.10f %.10f %.10f\n", brick.x, brick.y, brick.z, point.x, point.y, point.z);
   OutputDebugStringA(buf);
#endif

   if (fabs(normal.x - 1.0f) < k_epsilon) {
      brick.x -= 1;
   }
   if (fabs(normal.y - 1.0f) < k_epsilon) {
      brick.y -= 1;
   }
   if (fabs(normal.z - 1.0f) < k_epsilon) {
      brick.z -= 1;
   }

   if (!grid->isEmpty(brick)) {
      sel.AddBlock(brick);
   }
}
#endif
#endif

void RenderGrid::OnSelected(om::Selection& sel, const math3d::ray3& ray,
                            const math3d::point3& intersection, const math3d::point3& normal)
{
   const om::GridPtr grid = GetGrid();
   if (grid) {
      math3d::ipoint3 brick;

      // Handle the x and z coordinates...
      for (int i = 0; i < 3; i += 2) {
         // We want to choose the brick that the mouse is currently over.  The
         // intersection point is actually a point on the surface.  So to get the
         // brick, we need to move in the opposite direction of the normal
         if (normal[i] > k_epsilon) {
            brick[i] = std::floor(intersection[i]);
         } else if (normal[i] < k_epsilon) {
            brick[i] = std::ceil(intersection[i]);
         } else {
            // The brick origin is at the center of mass.  Adding 0.5f to the
            // coordinate and flooring it should return a brick coordinate.
            brick[i] = (int)std::floor(intersection[i] + 0.5f);
         }
      }
      // handle the y coordinate.  the brick's origin is at y == 0
      if (normal.y > k_epsilon) {
         brick.y = std::floor(intersection.y - 0.1); // 0.1's significantly bigger than k_epsilon
      } else if (normal.y < k_epsilon) {
         brick.y = std::floor(intersection.y + 0.1); // 0.1's significantly bigger than k_epsilon
      } else {
         brick.y = std::floor(intersection.y);
      }

#if 0
      char buf[1024];
      sprintf(buf, "%d, %d, %d  - %.10f %.10f %.10f\n", brick.x, brick.y, brick.z, intersection.x, intersection.y, intersection.z);
      OutputDebugStringA(buf);
#endif

      if (!grid->isEmpty(brick)) {
         // xxx: we should probably stash the coordinate system of the grid, no?
         sel.AddBlock(brick);
      }
   }
}

