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
#include "resources/res_manager.h"
#include "client/client.h"
#include "Horde3D.h"
#include <unordered_map>

using namespace ::radiant;
using namespace ::radiant::client;

#define T_LOG(level)      LOG(renderer.terrain, level)

static const csg::Point3 TERRAIN_LAYER_SIZE(256, 500, 256);
static const int MAX_CLIP_HEIGHT = 1000000000; // don't use INT_MAX due to overflow

RenderTerrain::RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain) :
   entity_(entity),
   terrain_(terrain),
   _tileSize(terrain->GetTileSize()),
   _clip_height(MAX_CLIP_HEIGHT),
   _enable_xray_mode(false)
{  
   terrain_root_node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "terrain root node"));
   selected_guard_ = Renderer::GetInstance().SetSelectionForNode(terrain_root_node_.get(), entity_.GetEntity());

   // terrain tiles are keyed in index space, but render terrain tiles are keyed in world space
   // i.e. they differ in scale by _tileSize
   auto on_add_tile = [this](csg::Point3 index, om::Region3BoxedPtr const& region) {
      csg::Point3 location = IndexToLocation(index);

      ASSERT(region);
      ASSERT(!stdutil::contains(tiles_, location));

      tiles_.insert(std::make_pair(location, new RenderTerrainTile(*this, location, region)));
      _dirtyNeighbors.insert(location);
      MarkDirty(location);
   };

   tiles_trace_ = terrain->TraceTiles("render", dm::RENDER_TRACES)
                              ->OnAdded(on_add_tile)
                              ->OnRemoved([this](csg::Point3 const& location) {
                                 NOT_YET_IMPLEMENTED();
                              })
                              ->PushObjectState();

   terrain_config_trace_ = terrain->TraceConfigFileName("render", dm::RENDER_TRACES)
                                    ->OnModified([this]() {
                                       LoadColorMap();
                                    })
                                    ->PushObjectState();

   std::shared_ptr<om::Region3MapWrapper> wrapper = std::make_shared<om::Region3MapWrapper>(_xray_region_tiles);
   _xray_tiles_accessor = std::make_shared<om::Region3Tiled>(_tileSize, wrapper);
}

RenderTerrain::~RenderTerrain()
{
   for (auto const& entry : tiles_) {
      delete entry.second;
   }
}

void RenderTerrain::LoadColorMap()
{
   _colorMap.clear();

   om::TerrainPtr terrainPtr = terrain_.lock();
   if (!terrainPtr) {
      return;
   }

   std::string config_file_name = terrainPtr->GetConfigFileName();
   if (config_file_name.empty()) {
      return;
   }

   res::ResourceManager2::GetInstance().LookupJson(config_file_name, [&](const json::Node& config) {
      json::Node block_types = config.get_node("block_types");

      for (json::Node const& block_type : block_types) {
         int tag = block_type.get<int>("tag");
         std::string color_code = block_type.get<std::string>("color");
         csg::Color4 color = csg::Color4::FromString(color_code);
         _colorMap[tag] = color;
      }
   });
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

   ScheduleUpdate();
}

void RenderTerrain::MarkAllDirty()
{
   for (auto const& entry : tiles_) {
      csg::Point3 location = entry.first;
      _dirtyGeometry.insert(location);
      _dirtyClipPlanes.insert(location);
      _dirtyLayers.insert(GetLayerAddressForLocation(location));
   }

   ScheduleUpdate();
}

void RenderTerrain::ScheduleUpdate()
{
   // If we haven't yet done so, schedule a callback to regenerate everything
   if (renderer_frame_trace_.Empty()) {
      renderer_frame_trace_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         Update();
      });
      ASSERT(!renderer_frame_trace_.Empty());
   }
}

csg::Point3 RenderTerrain::IndexToLocation(csg::Point3 const& index)
{
   csg::Point3 location = index.Scaled(_tileSize);
   return location;
}

csg::Point3 RenderTerrain::LocationToIndex(csg::Point3 const& location)
{
   csg::Point3 index(
         location.x / _tileSize.x,
         location.y / _tileSize.y,
         location.z / _tileSize.z
      );
   return index;
}

csg::Point3 RenderTerrain::GetLayerAddressForLocation(csg::Point3 const& location)
{
   return csg::GetChunkIndexSlow(location, TERRAIN_LAYER_SIZE).Scaled(TERRAIN_LAYER_SIZE);
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
   EachEntryIn(bounds, tiles_, _tileSize, cb);
}

void RenderTerrain::EachLayerIn(csg::Cube3 const& bounds, std::function<void(csg::Point3 const& location, RenderTerrainLayer* tile)> cb)
{
   EachEntryIn(bounds, layers_, TERRAIN_LAYER_SIZE, cb);
}

template <typename T>
void RenderTerrain::EachEntryIn(csg::Cube3 const& bounds, T const& map, csg::Point3 const& chunkSize,
                                std::function<void(csg::Point3 const& location, typename T::mapped_type value)> cb)
{
   csg::Cube3 chunkIndicies = csg::GetChunkIndexSlow(bounds, chunkSize);

   for (csg::Point3 const& cursor : csg::EachPoint(chunkIndicies)) {
      csg::Point3 location = cursor * chunkSize;
      auto i = map.find(location);
      if (i != map.end()) {
         cb(location, i->second);
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

   om::TerrainPtr terrainPtr = terrain_.lock();
   if (!terrainPtr) {
      return;
   }

   csg::Cube3 slice = csg::ToInt(terrainPtr->GetBounds());

   // Dirty every possibly-affected tile.
   slice.min.y = std::min(old_clip_height, _clip_height) - 1;
   slice.max.y = (int)terrainPtr->GetBounds().max.y;
   EachTileIn(slice, [this](csg::Point3 const& cursor, RenderTerrainTile* tile) {
      MarkDirty(cursor);
   });

   // Adjust the visibility of every possible-affected layer.
   int terrain_max_y = (int)terrainPtr->GetBounds().max.y;
   slice.min.y = std::min(_clip_height, old_clip_height) - 1;
   slice.max.y = terrain_max_y;
   EachLayerIn(slice, [this](csg::Point3 const& cursor, RenderTerrainLayer* layer) {
      bool visible = cursor.y <= _clip_height;
      SetLayerVisible(*layer, visible);
   });
}

void RenderTerrain::EnableXrayMode(bool enabled)
{
   if (enabled == _enable_xray_mode) {
      return;
   }
   _enable_xray_mode = enabled;

   MarkAllDirty();
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
         std::shared_ptr<csg::Region3> xray_tile = nullptr;

         if (_enable_xray_mode) {
            // terrain, interior, and xray tiles are all stored in index space
            csg::Point3 index = LocationToIndex(location);
            xray_tile = _xray_tiles_accessor->GetTile(index);
         }

         int planesChanged = i->second->UpdateClipPlanes(_clip_height, xray_tile);
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
         std::shared_ptr<csg::Region3> xray_tile = nullptr;

         if (_enable_xray_mode) {
            // terrain, interior, and xray tiles are all stored in index space
            csg::Point3 index = LocationToIndex(location);
            xray_tile = _xray_tiles_accessor->GetTile(index);
         }

         i->second->UpdateGeometry(_clip_height, xray_tile);
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

   bool visible = LayerIsVisible(location);
   SetLayerVisible(layer, visible);
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

bool RenderTerrain::LayerIsVisible(csg::Point3 const& location) const
{
   bool visible = location.y < _clip_height;
   return visible;
}

void RenderTerrain::SetLayerVisible(RenderTerrainLayer& layer, bool visible)
{
   RenderNodePtr node = layer.GetNode();
   // node is null when the mesh is empty
   if (node) {
      node->SetVisible(visible);
   }
}

om::Region3TiledPtr RenderTerrain::GetXrayTiles()
{
   return _xray_tiles_accessor;
}
