local Array2D = require 'services.server.world_generation.array_2D'
local MathFns = require 'services.server.world_generation.math.math_fns'
local FilterFns = require 'services.server.world_generation.filter.filter_fns'
local TerrainType = require 'services.server.world_generation.terrain_type'
local TerrainInfo = require 'services.server.world_generation.terrain_info'
local BlueprintGenerator = require 'services.server.world_generation.blueprint_generator'
local MicroMapGenerator = require 'services.server.world_generation.micro_map_generator'
local Landscaper = require 'services.server.world_generation.landscaper'
local TerrainGenerator = require 'services.server.world_generation.terrain_generator'
local HeightMapRenderer = require 'services.server.world_generation.height_map_renderer'
local HabitatType = require 'services.server.world_generation.habitat_type'
local HabitatManager = require 'services.server.world_generation.habitat_manager'
local OverviewMap = require 'services.server.world_generation.overview_map'
local Timer = require 'services.server.world_generation.timer'
local RandomNumberGenerator = _radiant.csg.RandomNumberGenerator
local Point3 = _radiant.csg.Point3

local WorldGenerationService = class()
local log = radiant.log.create_logger('world_generation')

function WorldGenerationService:__init()
end

function WorldGenerationService:initialize()
   log:write(0, 'initialize not implemented for world generation service!')
end

function WorldGenerationService:create_new_game(seed, async)
   self:set_seed(seed)
   self._async = async
   self._enable_scenarios = radiant.util.get_config('enable_scenarios', true)

   self._terrain_info = TerrainInfo()
   self._tile_size = self._terrain_info.tile_size
   self._macro_block_size = self._terrain_info.macro_block_size
   self._feature_size = self._terrain_info.feature_size

   self._micro_map_generator = MicroMapGenerator(self._terrain_info, self._rng)
   self._terrain_generator = TerrainGenerator(self._terrain_info, self._rng, self._async)
   self._height_map_renderer = HeightMapRenderer(self._terrain_info)
   self._landscaper = Landscaper(self._terrain_info, self._rng, self._async)

   self._scenario_service = stonehearth.scenario
   self._scenario_service:create_new_game(self._feature_size, seed)
   self._habitat_manager = HabitatManager(self._terrain_info, self._landscaper)

   self.blueprint_generator = BlueprintGenerator()
   self.overview_map = OverviewMap(self._terrain_info, self._landscaper)

   self._starting_location = nil
end

function WorldGenerationService:set_seed(seed)
   log:info('WorldGenerationService using seed %d', seed)
   self._seed = seed
   self._rng = RandomNumberGenerator(self._seed)
end

function WorldGenerationService:_report_progress(progress)
   radiant.events.trigger(radiant, 'stonehearth:generate_world_progress', {
      progress = progress * 100
   })
end

-- set and populate the blueprint
function WorldGenerationService:set_blueprint(blueprint)
   local seconds = Timer.measure(
      function()
         local tile_size = self._tile_size
         local macro_blocks_per_tile = self._tile_size / self._macro_block_size
         local blueprint_generator = self.blueprint_generator
         local micro_map_generator = self._micro_map_generator
         local landscaper = self._landscaper
         local full_micro_map, full_elevation_map, full_feature_map, full_habitat_map

         full_micro_map, full_elevation_map = micro_map_generator:generate_micro_map(blueprint)

         full_feature_map = Array2D(full_elevation_map.width, full_elevation_map.height)

         -- determine which features will be placed in which cells
         --landscaper:mark_boulders(full_elevation_map, full_feature_map)
         landscaper:mark_trees(full_elevation_map, full_feature_map)
         landscaper:mark_berry_bushes(full_elevation_map, full_feature_map)
         landscaper:mark_flowers(full_elevation_map, full_feature_map)

         full_habitat_map = self._habitat_manager:derive_habitat_map(full_elevation_map, full_feature_map)

         -- shard the maps and store in the blueprint
         -- micro_maps are overlapping so they need a different sharding function
         blueprint_generator:store_micro_map(blueprint, full_micro_map, macro_blocks_per_tile)
         blueprint_generator:shard_and_store_map(blueprint, "elevation_map", full_elevation_map)
         blueprint_generator:shard_and_store_map(blueprint, "feature_map", full_feature_map)
         blueprint_generator:shard_and_store_map(blueprint, "habitat_map", full_habitat_map)

         -- location of the world origin in the coordinate system of the blueprint
         blueprint.origin_x = math.floor(blueprint.width * tile_size / 2)
         blueprint.origin_y = math.floor(blueprint.height * tile_size / 2)

         -- create the overview map
         self.overview_map:derive_overview_map(full_elevation_map, full_feature_map,
                                               blueprint.origin_x, blueprint.origin_y)

         self._blueprint = blueprint
      end
   )
   log:info('Blueprint population time: %.3fs', seconds)
end

function WorldGenerationService:get_blueprint()
   return self._blueprint
end

function WorldGenerationService:set_starting_location(location)
   self._starting_location = location
   self._scenario_service:set_starting_location(location)

   self:_place_scenarios_on_generated_tiles()
end

function WorldGenerationService:_place_scenarios_on_generated_tiles()
   assert(self._blueprint)
   local blueprint = self._blueprint
   local tile_info, habitat_map, elevation_map, offset_x, offset_y

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         offset_x, offset_y = self:get_tile_origin(i, j, blueprint)

         tile_info = blueprint:get(i, j)
         habitat_map = tile_info.habitat_map
         elevation_map = tile_info.elevation_map

         self._scenario_service:place_revealed_scenarios(habitat_map, elevation_map, offset_x, offset_y)
      end
   end
end

-- get the (i,j) index of the blueprint tile for the world coordinates (x,y)
function WorldGenerationService:get_tile_index(x, y)
   local blueprint = self._blueprint
   local tile_size = self._tile_size
   local i = math.floor((x + blueprint.origin_x) / tile_size) + 1
   local j = math.floor((y + blueprint.origin_y) / tile_size) + 1
   return i, j
end

-- get the world coordinates of the origin (top-left corner) of the tile
function WorldGenerationService:get_tile_origin(i, j, blueprint)
   local x, y
   local tile_size = self._tile_size

   x = (i-1)*tile_size - blueprint.origin_x
   y = (j-1)*tile_size - blueprint.origin_y

   return x, y
end

function WorldGenerationService:generate_all_tiles()
   self:_run_async(
      function()
         local blueprint = self._blueprint
         local i, j, n, tile_order_list, num_tiles
         local progress = 0

         self:_report_progress(progress)

         tile_order_list = self:_build_tile_order_list(blueprint)
         num_tiles = #tile_order_list

         for n=1, num_tiles do
            i = tile_order_list[n].x
            j = tile_order_list[n].y

            self:_generate_tile_impl(i, j)

            progress = n / num_tiles
            self:_report_progress(progress)
         end
      end
   )
end

function WorldGenerationService:generate_tiles(i, j, radius)
   self:_run_async(
      function()
         local blueprint = self._blueprint
         local x_min = math.max(i-radius, 1)
         local x_max = math.min(i+radius, blueprint.width)
         local y_min = math.max(j-radius, 1)
         local y_max = math.min(j+radius, blueprint.height)
         local num_tiles = (x_max-x_min+1) * (y_max-y_min+1)
         local n = 0
         local progress = 0

         self:_report_progress(progress)

         for b=y_min, y_max do
            for a=x_min, x_max do
               assert(blueprint:in_bounds(a, b))

               self:_generate_tile_impl(a, b)

               n = n + 1
               progress = n / num_tiles
               self:_report_progress(progress)
            end
         end
      end
   )
end

function WorldGenerationService:generate_tile(i, j)
   self:_run_async(
      function()
         self:_generate_tile_impl(i, j)
      end
   )
end

function WorldGenerationService:_generate_tile_impl(i, j)
   local blueprint = self._blueprint
   local tile_size = self._tile_size
   local tile_map, tile_info, tile_seed
   local micro_map, elevation_map, feature_map, habitat_map
   local offset_x, offset_y

   tile_info = blueprint:get(i, j)
   assert(not tile_info.generated)

   log:info('Generating tile (%d,%d) - %s', i, j, tile_info.terrain_type)

   -- calculate the world offset of the tile
   offset_x, offset_y = self:get_tile_origin(i, j, blueprint)

   -- make each tile deterministic on its coordinates (and game seed)
   tile_seed = self:_get_tile_seed(i, j)
   self._rng:set_seed(tile_seed)

   -- get the various maps from the blueprint
   micro_map = tile_info.micro_map
   elevation_map = tile_info.elevation_map
   feature_map = tile_info.feature_map
   habitat_map = tile_info.habitat_map

   -- generate the high resolution heightmap for the tile
   local seconds = Timer.measure(
      function()
         tile_map = self._terrain_generator:generate_tile(micro_map)
      end
   )
   log:info('Terrain generation time: %.3fs', seconds)
   self:_yield()

   -- render heightmap to region3
   self:_render_heightmap_to_region3(tile_map, feature_map, offset_x, offset_y)

   -- place flora
   self:_place_flora(tile_map, feature_map, offset_x, offset_y)

   -- place scenarios
   self:_place_scenarios(habitat_map, elevation_map, offset_x, offset_y)

   tile_info.generated = true
end

function WorldGenerationService:_render_heightmap_to_region3(tile_map, feature_map, offset_x, offset_y)
   local offset = Point3(offset_x, 0, offset_y)
   local renderer = self._height_map_renderer
   local region3_boxed

   local seconds = Timer.measure(
      function()
         region3_boxed = renderer:create_new_region()
         renderer:render_height_map_to_region(region3_boxed, tile_map)
         --self._landscaper:place_boulders(region3_boxed, tile_map, feature_map)
         renderer:add_region_to_terrain(region3_boxed, offset)
      end
   )

   log:info('Height map to region3 time: %.3fs', seconds)
   self:_yield()
end

function WorldGenerationService:_place_flora(tile_map, feature_map, offset_x, offset_y)
   local seconds = Timer.measure(
      function()
         self._landscaper:place_flora(tile_map, feature_map, offset_x, offset_y)
      end
   )

   log:info('Landscaper time: %.3fs', seconds)
   self:_yield()
end

function WorldGenerationService:_place_scenarios(habitat_map, elevation_map, offset_x, offset_y)
   if not self._enable_scenarios then
      return
   end

   local seconds = Timer.measure(
      function()
         self._scenario_service:place_static_scenarios(habitat_map, elevation_map, offset_x, offset_y)

         if self._starting_location then
            -- add scenarios as tiles are generated
            -- if this is before the starting location is determined, we will add them when set_starting_location is called
            self._scenario_service:place_revealed_scenarios(habitat_map, elevation_map, offset_x, offset_y)
         end
      end
   )

   log:info('Static scenario time: %.3fs', seconds)
   self:_yield()
end

function WorldGenerationService:_get_tile_seed(x, y)
   local tile_hash = MathFns.point_hash(x, y)
   return (self._seed + tile_hash) % MathFns.MAX_UINT32
end

function WorldGenerationService:_build_tile_order_list(map)
   local center_x = (map.width+1)/2 -- center can be non-integer
   local center_y = (map.height+1)/2
   local tile_order = {}
   local i, j, dx, dy, coord_info, angle

   for j=1, map.height do
      for i=1, map.width do
         coord_info = {}
         coord_info.x = i
         coord_info.y = j
         dx = i-center_x
         dy = j-center_y

         -- break ties in radial order
         angle = self:_get_angle(dy, dx)
         coord_info.dist_metric = dx*dx + dy*dy + angle/1000

         table.insert(tile_order, coord_info)
      end
   end

   local compare_tile = function(a, b)
      return a.dist_metric < b.dist_metric
   end
   table.sort(tile_order, compare_tile)
   return tile_order
end

function WorldGenerationService:_get_angle(dy, dx)
   local pi = math.pi

   -- normalize angle to a range of 0 - 2pi
   local value = math.atan2(dy, dx) + pi

   -- move minimum to 45 degrees (pi/4) so fill order looks better
   if value < pi/4 then value = value + 2*pi end
   return value
end

function WorldGenerationService:_run_async(fn)
   if self._async then
      radiant.create_background_task('World Generation', fn)
   else
      fn()
   end
end

function WorldGenerationService:_yield()
   if self._async then
      coroutine.yield()
   end
end

return WorldGenerationService
