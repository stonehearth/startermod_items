#include "pch.h"
#include "radiant_stdutil.h"
#include "pipeline.h"
#include "renderer.h"
#include "render_terrain.h"
#include "render_terrain_tile.h"
#include "render_terrain_layer.h"
#include "dm/map_trace.h"
#include "om/components/terrain.ridl.h"
#include "csg/iterators.h"
#include "csg/meshtools.h"
#include "lib/perfmon/perfmon.h"
#include "Horde3D.h"
#include <unordered_map>

using namespace ::radiant;
using namespace ::radiant::client;

#define T_LOG(level)      LOG(renderer.terrain, level)

static const csg::Point3 TERRAIN_LAYER_SIZE(128, 5, 128);


RenderTerrain::RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain) :
   entity_(entity),
   terrain_(terrain),
   _tileSize(terrain->GetTileSize()),
   _clip_height(INT_MAX)
{  
   terrain_root_node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "terrain root node"));
   selected_guard_ = Renderer::GetInstance().SetSelectionForNode(terrain_root_node_.get(), entity_.GetEntity());

   if (_colorMap.empty()) {
      InitalizeColorMap();
   }
   auto on_add_tile = [this](csg::Point3 index, om::Region3BoxedPtr const& region) {
      csg::Point3 location = index.Scaled(_tileSize);

      ASSERT(region);
      ASSERT(!stdutil::contains(tiles_, location));

      tiles_.insert(std::make_pair(location, new RenderTerrainTile(*this, location, region)));
      _dirtyNeighbors.insert(location);
      MarkDirty(location);
   };

   auto on_remove_tile = 

   tiles_trace_ = terrain->TraceTiles("render", dm::RENDER_TRACES)
                              ->OnAdded(on_add_tile)
                              ->OnRemoved([this](csg::Point3 const& location) {
                                 NOT_YET_IMPLEMENTED();
                              })
                              ->PushObjectState();
}

RenderTerrain::~RenderTerrain()
{
   for (auto const& entry : tiles_) {
      delete entry.second;
   }
}

void RenderTerrain::InitalizeColorMap()
{
   std::string const unknownColor = "#ff00ff";
   json::Node config = Renderer::GetInstance().GetTerrainConfig();
   _colorMap[om::Terrain::Hidden]     = csg::Color4::FromString(config.get("hidden", unknownColor));
   _colorMap[om::Terrain::Bedrock]    = csg::Color4::FromString(config.get("bedrock", unknownColor));
   _colorMap[om::Terrain::RockLayer1] = csg::Color4::FromString(config.get("rock.layer_1", unknownColor));
   _colorMap[om::Terrain::RockLayer2] = csg::Color4::FromString(config.get("rock.layer_2", unknownColor));
   _colorMap[om::Terrain::RockLayer3] = csg::Color4::FromString(config.get("rock.layer_3", unknownColor));
   _colorMap[om::Terrain::RockLayer4] = csg::Color4::FromString(config.get("rock.layer_4", unknownColor));
   _colorMap[om::Terrain::RockLayer5] = csg::Color4::FromString(config.get("rock.layer_5", unknownColor));
   _colorMap[om::Terrain::RockLayer6] = csg::Color4::FromString(config.get("rock.layer_6", unknownColor));
   _colorMap[om::Terrain::SoilLight]  = csg::Color4::FromString(config.get("soil.light", unknownColor));
   _colorMap[om::Terrain::SoilDark]   = csg::Color4::FromString(config.get("soil.dark", unknownColor));
   _colorMap[om::Terrain::GrassEdge1] = csg::Color4::FromString(config.get("grass.edge1", unknownColor));
   _colorMap[om::Terrain::GrassEdge2] = csg::Color4::FromString(config.get("grass.edge2", unknownColor));
   _colorMap[om::Terrain::Grass]      = csg::Color4::FromString(config.get("grass.inner", unknownColor));
   _colorMap[om::Terrain::DirtEdge1]  = csg::Color4::FromString(config.get("dirt.edge1", unknownColor));
   _colorMap[om::Terrain::Dirt]       = csg::Color4::FromString(config.get("dirt.inner", unknownColor));
}

csg::Point3 RenderTerrain::GetLayerAddressForLocation(csg::Point3 const& location)
{
   return csg::GetChunkIndexSlow(location, TERRAIN_LAYER_SIZE).Scaled(TERRAIN_LAYER_SIZE);
}

void RenderTerrain::MarkDirty(csg::Point3 const& location)
{
   // The geometry and contents of the clip planes at this location clearly need to be
   // regenerated, which means adding the location to those 3 sets.
   _dirtyGeometry.insert(location);
   _dirtyClipPlanes.insert(location);
   _dirtyLayers.insert(GetLayerAddressForLocation(location));

   // Also, though it's shape hasn't changed, we may need to generate (or remove) geometry from
   // all tiles adjacent to this one, so stick those in dirty geometry and layer buckets, too.
   for (int d = 0; d < csg::RegionTools3::NUM_PLANES; d++) {
      csg::RegionTools3::Plane direction = static_cast<csg::RegionTools3::Plane>(d);
      csg::Point3 neighbor = GetNeighborAddress(location, direction);
      _dirtyGeometry.insert(neighbor);
      _dirtyLayers.insert(GetLayerAddressForLocation(neighbor));
   }

   // If we haven't yet done so, schedule a callback to regenerate everything
   if (renderer_frame_trace_.Empty()) {
      renderer_frame_trace_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         Update();
      });
      ASSERT(!renderer_frame_trace_.Empty());
   }
}

csg::Point3 RenderTerrain::GetNeighborAddress(csg::Point3 const& location, csg::RegionTools3::Plane direction)
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);
   static csg::Point3 offsets[] = {
      csg::Point3(0, -_tileSize.y, 0),    // BOTTOM_PLANE
      csg::Point3(0, _tileSize.y, 0),     // TOP_PLANE
      csg::Point3(-_tileSize.x, 0, 0),    // LEFT_PLANE
      csg::Point3(_tileSize.x, 0, 0),     // RIGHT_PLANE
      csg::Point3(0, 0, -_tileSize.z),    // FRONT_PLANE
      csg::Point3(0, 0, _tileSize.z),     // BACK_PLANE
   };
   return location + offsets[direction];
}

void RenderTerrain::EachTileIn(csg::Cube3 const& bounds, std::function<void(csg::Point3 const& location, RenderTerrainTile* tile)> cb)
{
   csg::Cube3 regionChunks = csg::GetChunkIndexSlow(bounds, _tileSize);

   for (csg::Point3 const& cursor : csg::EachPoint(regionChunks)) {
      csg::Point3 tilePoint = cursor * _tileSize;
      auto i = tiles_.find(tilePoint);
      if (i != tiles_.end()) {
         cb(tilePoint, i->second);
      }
   }
}

void RenderTerrain::ConnectNeighbors(csg::Point3 const& location, RenderTerrainTile& first, csg::RegionTools3::Plane direction)
{
   ASSERT(direction >= 0 && direction < csg::RegionTools3::NUM_PLANES);

   auto i = tiles_.find(GetNeighborAddress(location, direction));
   if (i != tiles_.end()) {
      RenderTerrainTile& second = *i->second;
      csg::RegionTools3::Plane neighbor = csg::RegionTools3::GetNeighbor(direction);
      first.SetClipPlane(direction, second.GetClipPlane(neighbor));
      second.SetClipPlane(neighbor, first.GetClipPlane(direction));
   }
}

void RenderTerrain::AddCut(om::Region3fBoxedPtr const& cut) 
{
   if (_cutToICut.find(cut->GetObjectId()) != _cutToICut.end()) {
      return;
   }
   _cutToICut[cut->GetObjectId()] = csg::ToInt(cut->Get());

   auto trace = cut->TraceChanges("cut region change", dm::RENDER_TRACES);
   om::Region3fBoxedRef cutRef = cut;
   _cut_trace_map[cut->GetObjectId()] = trace;

   trace->OnChanged([cutRef, this](csg::Region3f const& region) {
      auto cut = cutRef.lock();

      if (!cut) {
         return;
      }
      dm::ObjectId objId = cut->GetObjectId();

      // Update the cut map to the new region.
      EachTileIn(_cutToICut[objId].GetBounds(), [this, objId](csg::Point3 const& cursor, RenderTerrainTile* tile) {
         tile->RemoveCut(objId);
         MarkDirty(cursor);
      });

      // Now, update the stored region, and find the new overlapping tiles.
      _cutToICut[objId] = csg::ToInt(region);
      csg::Region3 const* iRegion = &_cutToICut[objId];
      EachTileIn(iRegion->GetBounds(), [this, objId, iRegion](csg::Point3 const& cursor, RenderTerrainTile* tile) {
         tile->AddCut(objId, iRegion);
         MarkDirty(cursor);
      });

   })->PushObjectState();
}

void RenderTerrain::RemoveCut(om::Region3fBoxedPtr const& cut)
{
   dm::ObjectId objId = cut->GetObjectId();
   if (_cutToICut.find(objId) == _cutToICut.end()) {
      return;
   }

   _cut_trace_map.erase(cut->GetObjectId());
   EachTileIn(_cutToICut[objId].GetBounds(), [this, objId](csg::Point3 const& cursor, RenderTerrainTile* tile) {
      tile->RemoveCut(objId);
      MarkDirty(cursor);
   });
   _cutToICut.erase(objId);
}

void RenderTerrain::SetClipHeight(int height)
{
   if (height == _clip_height) {
      return;
   }

   int old_clip_height = _clip_height;
   _clip_height = height;

   for (auto const& entry : tiles_) {
      int min_y = entry.first.y;
      int max_y = min_y + _tileSize.y;
      // mark dirty all tiles affected by the previous and current clip heights
      if (csg::IsBetween(min_y, old_clip_height, max_y) || csg::IsBetween(min_y, _clip_height, max_y)) {
         // technically, we only need to mark the geometry dirty, but this is convenient
         MarkDirty(entry.first);
      }
   }
}

void RenderTerrain::UpdateNeighbors()
{   
   for (csg::Point3 const& location : _dirtyNeighbors) {
      auto i = tiles_.find(location);
      if (i != tiles_.end()) {
         RenderTerrainTile& tile = *i->second;
         for (int d = 0; d < csg::RegionTools3::NUM_PLANES; d++) {
            csg::RegionTools3::Plane direction = static_cast<csg::RegionTools3::Plane>(d);
            ConnectNeighbors(location, tile, direction);
         }
      }
   }
   _dirtyNeighbors.clear();
}

void RenderTerrain::UpdateClipPlanes()
{
   for (csg::Point3 const& location : _dirtyClipPlanes) {
      auto i = tiles_.find(location);
      if (i != tiles_.end()) {
         int planesChanged = i->second->UpdateClipPlanes(_clip_height);
         for (int d = 0; d < csg::RegionTools3::NUM_PLANES; d++) {
            csg::RegionTools3::Plane direction = static_cast<csg::RegionTools3::Plane>(d);
            if (planesChanged & (1 << d)) {
               _dirtyGeometry.insert(GetNeighborAddress(location, direction));
            }
         }
      }
   }
   _dirtyClipPlanes.clear();
}


void RenderTerrain::UpdateGeometry()
{   
   for (csg::Point3 const& location : _dirtyGeometry) {
      auto i = tiles_.find(location);
      if (i != tiles_.end()) {
         i->second->UpdateGeometry(_clip_height);
      }
   }
   _dirtyGeometry.clear();
}

void RenderTerrain::UpdateLayer(RenderTerrainLayer &layer, csg::Point3 const& location)
{
   csg::Point3 index;
   csg::Point3 end = location + TERRAIN_LAYER_SIZE;

   layer.BeginUpdate();

   // Run through every tile in the layer and throw it's geometry into the mesh
   for (index.y = location.y; index.y < end.y; index.y += _tileSize.y) {
      for (index.x = location.x; index.x < end.x; index.x += _tileSize.x) {
         for (index.z = location.z; index.z < end.z; index.z += _tileSize.z) {
            auto i = tiles_.find(index);
            if (i != tiles_.end()) {
               RenderTerrainTile& tile = *i->second;
               for (int d = 0; d < csg::RegionTools3::NUM_PLANES; d++) {
                  csg::RegionTools3::Plane direction = static_cast<csg::RegionTools3::Plane>(d);
                  layer.AddGeometry(direction, tile.GetGeometry(direction));
               }
            }            
         }
      }
   }
   layer.EndUpdate();
}

void RenderTerrain::UpdateLayers()
{
   for (csg::Point3 const& location : _dirtyLayers) {
      RenderTerrainLayer &layer = GetLayer(location);
      UpdateLayer(layer, location);
   }
   _dirtyLayers.clear();
}

void RenderTerrain::Update()
{
   UpdateClipPlanes();
   UpdateNeighbors();
   UpdateGeometry();
   UpdateLayers();
   renderer_frame_trace_.Clear();
}

csg::TagToColorMap const& RenderTerrain::GetColorMap() const
{
   return _colorMap;
}

H3DNode RenderTerrain::GetGroupNode() const
{
   return terrain_root_node_.get();
}

csg::Point3 RenderTerrain::GetTileSize()
{
   return _tileSize;
}

RenderTerrainLayer& RenderTerrain::GetLayer(csg::Point3 const& location)
{
   auto i = layers_.find(location);
   if (i != layers_.end()) {
      return *i->second;
   }
   auto j = layers_.insert(std::make_pair(location, new RenderTerrainLayer(*this, location)));
   return *j.first->second;
}
