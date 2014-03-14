local Array2D = require 'services.world_generation.array_2D'
local TerrainType = require 'services.world_generation.terrain_type'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local MathFns = require 'services.world_generation.math.math_fns'
local Histogram = require 'services.world_generation.math.histogram'
local RandomNumberGenerator = _radiant.csg.RandomNumberGenerator

local BlueprintGenerator = class()
local log = radiant.log.create_logger('world_generation')

function BlueprintGenerator:__init()
end

-- (6, 6) is the minimum size for generated blueprints
function BlueprintGenerator:generate_blueprint(width, height, seed)
   -- minimum size for this algorithm
   assert(width >= 5)
   assert(height >= 5)

   local mountains_threshold = 65
   local foothills_threshold = 50
   local rng = RandomNumberGenerator(seed)
   local blueprint = self:get_empty_blueprint(width, height)
   local noise_map = Array2D(width, height)
   local height_map = Array2D(width, height)
   local i, j, value, terrain_type

   local noise_fn = function(i, j)
      return rng:get_gaussian(55, 50)
   end

   noise_map:fill_ij(noise_fn)
   FilterFns.filter_2D_050(height_map, noise_map, noise_map.width, noise_map.height, 6)

   for j=1, height do
      for i=1, width do
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

function BlueprintGenerator:store_micro_map(blueprint, full_micro_map, macro_blocks_per_tile)
   local local_micro_map

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         -- +1 for the margins
         local_micro_map = Array2D(macro_blocks_per_tile+1, macro_blocks_per_tile+1)

         Array2D.copy_block(local_micro_map, full_micro_map, 1, 1,
            (i-1)*macro_blocks_per_tile+1, (j-1)*macro_blocks_per_tile+1,
            macro_blocks_per_tile+1, macro_blocks_per_tile+1)

         blueprint:get(i, j).micro_map = local_micro_map
      end
   end
end

function BlueprintGenerator:shard_and_store_map(blueprint, key, full_map)
   local features_per_tile, local_map

   features_per_tile = full_map.width / blueprint.width
   assert(features_per_tile == full_map.height / blueprint.height)
   assert(features_per_tile % 1 == 0) -- assert is integer

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         local_map = Array2D(features_per_tile, features_per_tile)

         Array2D.copy_block(local_map, full_map, 1, 1,
            (i-1)*features_per_tile+1, (j-1)*features_per_tile+1,
            features_per_tile, features_per_tile)

         blueprint:get(i, j)[key] = local_map
      end
   end
end

return BlueprintGenerator
