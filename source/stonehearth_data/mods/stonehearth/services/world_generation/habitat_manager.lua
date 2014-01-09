local Array2D = require 'services.world_generation.array_2D'
local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local HabitatType = require 'services.world_generation.habitat_type'
local log = radiant.log.create_logger('world_generation')

local HabitatManager = class()

function HabitatManager:__init(terrain_info, landscaper)
   self._terrain_info = terrain_info
   self._landscaper = landscaper
end

function HabitatManager:derive_habitat_map(feature_map, micro_map)
   -- -1 for the edge overlap, *2 for scale
   assert(feature_map.width == (micro_map.width-1)*2)
   assert(feature_map.height == (micro_map.height-1)*2)

   local terrain_info = self._terrain_info
   local habitat_map = Array2D(feature_map.width, feature_map.height)
   local elevation_map = Array2D(feature_map.width, feature_map.height)
   local i, j, a, b, feature_name, elevation, terrain_type, habitat_type

   for j=1, habitat_map.height do
      for i=1, habitat_map.width do
         a, b = _habitat_map_to_micro_map_coords(i, j)
         elevation = micro_map:get(a, b)
         elevation_map:set(i, j, elevation)
         terrain_type = terrain_info:get_terrain_type(elevation)
         feature_name = feature_map:get(i, j)
         habitat_type = self:_get_habitat_type(terrain_type, feature_name)
         habitat_map:set(i, j, habitat_type)
      end
   end

   return habitat_map, elevation_map
end

-- This is likely to change
function HabitatManager:_get_habitat_type(terrain_type, feature_name)
   if terrain_type == TerrainType.Mountains then
      return HabitatType.Mountains
   end
   if self._landscaper:is_forest_feature(feature_name) then
      return HabitatType.Forest
   end
   if feature_name ~= nil then
      return HabitatType.Occupied
   end
   if terrain_type == TerrainType.Grassland then
      return HabitatType.Grassland
   end
   if terrain_type == TerrainType.Foothills then
      return HabitatType.Foothills
   end
   log:error('Unable to deduce HabitatType')
   return HabitatType.Occupied
end

function _habitat_map_to_micro_map_coords(i, j)
   return _habitat_map_to_micro_map_index(i), _habitat_map_to_micro_map_index(j)
end

-- recall that micro_maps are offset by half a macroblock for shared tiling boundaries
function _habitat_map_to_micro_map_index(i)
   -- offset 1 element to skip over micro_map overlap
   -- convert to 0 based array, scale, convert back to 1 based array
   return math.floor((i+1-1) * 0.5) + 1
end

return HabitatManager
