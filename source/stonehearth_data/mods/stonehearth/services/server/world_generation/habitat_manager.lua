local Array2D = require 'services.server.world_generation.array_2D'
local TerrainType = require 'services.server.world_generation.terrain_type'
local TerrainInfo = require 'services.server.world_generation.terrain_info'
local HabitatType = require 'services.server.world_generation.habitat_type'
local log = radiant.log.create_logger('world_generation')

local HabitatManager = class()

function HabitatManager:__init(terrain_info, landscaper)
   self._terrain_info = terrain_info
   self._landscaper = landscaper
end

function HabitatManager:derive_habitat_map(elevation_map, feature_map)
   local terrain_info = self._terrain_info
   local habitat_map = Array2D(feature_map.width, feature_map.height)
   local i, j, feature_name, elevation, terrain_type, habitat_type

   for j=1, habitat_map.height do
      for i=1, habitat_map.width do
         elevation = elevation_map:get(i, j)
         terrain_type = terrain_info:get_terrain_type(elevation)

         feature_name = feature_map:get(i, j)
         habitat_type = self:_get_habitat_type(terrain_type, feature_name)

         habitat_map:set(i, j, habitat_type)
      end
   end

   return habitat_map
end

-- This is likely to change
function HabitatManager:_get_habitat_type(terrain_type, feature_name)
   if terrain_type == TerrainType.mountains then
      return HabitatType.mountains
   end
   if self._landscaper:is_forest_feature(feature_name) then
      return HabitatType.forest
   end
   if feature_name ~= nil then
      return HabitatType.occupied
   end
   if terrain_type == TerrainType.plains then
      return HabitatType.plains
   end
   if terrain_type == TerrainType.foothills then
      return HabitatType.foothills
   end
   log:error('Unable to deduce HabitatType')
   return HabitatType.occupied
end

return HabitatManager
