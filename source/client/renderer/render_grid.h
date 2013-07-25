#ifndef _RADIANT_CLIENT_RENDER_GRID_H
#define _RADIANT_CLIENT_RENDER_GRID_H

#include <unordered_map>
#include "namespace.h"
#include "Horde3D.h"
#include "om/om.h"
#include "dm/dm.h"
#include "types.h"
#include "math3d.h"
#include "render_component.h"
#include "om/grid/tile_address.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderGrid : public RenderComponent {
public:
   RenderGrid(const RenderEntity& entity, om::RenderGridPtr renderGrid);
   ~RenderGrid();

private:
   void UpdateAllTiles();
   void AddDirtyTile(const om::TileAddress& addr, const om::GridTilePtr tile);
   void RemoveTile(const om::TileAddress& addr);
   void DestroyRenderNodes();
   void FlushDirtyTiles();
   void OnSelected(om::Selection& sel, const math3d::ray3& ray,
                   const math3d::point3& intersection, const math3d::point3& normal);

private:
   om::GridPtr GetGrid() const;

private:
   const RenderEntity&  entity_;
   dm::Guard              tracer_;
   om::RenderGridRef    renderGrid_;

   struct TileEntry {
      dm::Guard      traces;
      H3DNode        node;
      
      TileEntry(H3DNode n) : node(n) { }
      ~TileEntry() {
         if (node) {
            h3dRemoveNode(node);
         }
      }
   };
   typedef std::unordered_map<om::TileAddress, std::shared_ptr<TileEntry>, om::TileAddress::Hash> TileMap;
   typedef vector<om::TileAddress> DirtyTileVector;

   H3DNode                       node_;
   H3DRes                        texture_;
   H3DRes                        material_;
   TileMap                       _tiles;
   DirtyTileVector               _dirtyTiles;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_GRID_H
