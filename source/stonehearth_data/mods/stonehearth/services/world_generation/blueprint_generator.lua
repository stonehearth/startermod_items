local Array2D = require 'services.world_generation.array_2D'
local TerrainType = require 'services.world_generation.terrain_type'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local MathFns = require 'services.world_generation.math.math_fns'
local RandomNumberGenerator = _radiant.csg.RandomNumberGenerator

local BlueprintGenerator = class()
local log = radiant.log.create_logger('world_generation')

function BlueprintGenerator:__init(rng)
   self._rng = rng
end

function BlueprintGenerator:generate_blueprint(width, height)
   local mountains_threshold = 65
   local foothills_threshold = 50
   local blueprint = self:get_empty_blueprint(width, height)
   local noise_map = Array2D(width, height)
   local height_map = Array2D(width, height)
   local i, j, value, terrain_type

   local noise_fn = function(i, j)
      return self._rng:get_gaussian(55, 50)
   end

   while (true) do
      noise_map:fill_ij(noise_fn)
      FilterFns.filter_2D_050(height_map, noise_map, noise_map.width, noise_map.height, 4)

      --log:debug('blueprint noise map')
      --noise_map:print()
      --log:debug('blueprint height map')
      --height_map:print()

      for i=1, height do
         for j=1, width do
            value = height_map:get(i, j)
            if value >= mountains_threshold then
               terrain_type = TerrainType.mountains
            elseif value >= foothills_threshold then
               terrain_type = TerrainType.foothills
            else
               terrain_type = TerrainType.plains
            end
            blueprint:get(i, j).terrain_type = terrain_type
         end
      end

      -- need this for maps with small sample size
      if self:_is_playable_map(blueprint) then
         break
      end
   end

   return blueprint
end

function BlueprintGenerator:get_empty_blueprint(width, height, terrain_type)
   if terrain_type == nil then terrain_type = TerrainType.plains end

   local blueprint = Array2D(width, height)
   local i, j, tile_info

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         tile_info = {}
         tile_info.terrain_type = terrain_type
         tile_info.generated = false
         blueprint:set(i, j, tile_info)
      end
   end

   return blueprint
end

function BlueprintGenerator:_is_playable_map(blueprint)
   local i, j, terrain_type, percent_mountains
   local total_tiles = blueprint.width * blueprint.height
   local stats = {}

   stats[TerrainType.plains] = 0
   stats[TerrainType.foothills] = 0
   stats[TerrainType.mountains] = 0

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         terrain_type = blueprint:get(i, j).terrain_type
         stats[terrain_type] = stats[terrain_type] + 1
      end
   end

   log:debug('Terrain distribution:')
   log:debug('plains: %d, foothills: %d, mountains: %d', stats[TerrainType.plains],
      stats[TerrainType.foothills], stats[TerrainType.mountains])

   percent_mountains = stats[TerrainType.mountains] / total_tiles

   return MathFns.in_bounds(percent_mountains, 0.20, 0.40)
end

function BlueprintGenerator:get_static_blueprint()
   local blueprint = self:_get_empty_blueprint(5, 5)

   blueprint:get(1, 1).terrain_type = TerrainType.plains
   blueprint:get(2, 1).terrain_type = TerrainType.plains
   blueprint:get(3, 1).terrain_type = TerrainType.foothills
   blueprint:get(4, 1).terrain_type = TerrainType.mountains
   blueprint:get(5, 1).terrain_type = TerrainType.mountains

   blueprint:get(1, 2).terrain_type = TerrainType.plains
   blueprint:get(2, 2).terrain_type = TerrainType.plains
   blueprint:get(3, 2).terrain_type = TerrainType.plains
   blueprint:get(4, 2).terrain_type = TerrainType.foothills
   blueprint:get(5, 2).terrain_type = TerrainType.mountains

   blueprint:get(1, 3).terrain_type = TerrainType.foothills
   blueprint:get(2, 3).terrain_type = TerrainType.plains
   blueprint:get(3, 3).terrain_type = TerrainType.plains
   blueprint:get(4, 3).terrain_type = TerrainType.plains
   blueprint:get(5, 3).terrain_type = TerrainType.foothills

   blueprint:get(1, 4).terrain_type = TerrainType.mountains
   blueprint:get(2, 4).terrain_type = TerrainType.foothills
   blueprint:get(3, 4).terrain_type = TerrainType.foothills
   blueprint:get(4, 4).terrain_type = TerrainType.plains
   blueprint:get(5, 4).terrain_type = TerrainType.plains

   blueprint:get(1, 5).terrain_type = TerrainType.mountains
   blueprint:get(2, 5).terrain_type = TerrainType.mountains
   blueprint:get(3, 5).terrain_type = TerrainType.mountains
   blueprint:get(4, 5).terrain_type = TerrainType.foothills
   blueprint:get(5, 5).terrain_type = TerrainType.plains

   return blueprint
end

return BlueprintGenerator
