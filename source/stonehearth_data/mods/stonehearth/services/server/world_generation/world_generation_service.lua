local Array2D = require 'services.server.world_generation.array_2D'
local FilterFns = require 'services.server.world_generation.filter.filter_fns'
local TerrainInfo = require 'services.server.world_generation.terrain_info'
local BlueprintGenerator = require 'services.server.world_generation.blueprint_generator'
local MicroMapGenerator = require 'services.server.world_generation.micro_map_generator'
local Landscaper = require 'services.server.world_generation.landscaper'
local TerrainGenerator = require 'services.server.world_generation.terrain_generator'
local HeightMapRenderer = require 'services.server.world_generation.height_map_renderer'
local HabitatManager = require 'services.server.world_generation.habitat_manager'
local OverviewMap = require 'services.server.world_generation.overview_map'
local ScenarioIndex = require 'services.server.world_generation.scenario_index'
local OreScenarioSelector = require 'services.server.world_generation.ore_scenario_selector'
local SurfaceScenarioSelector = require 'services.server.world_generation.surface_scenario_selector'
local Timer = require 'services.server.world_generation.timer'
local RandomNumberGenerator = _radiant.csg.RandomNumberGenerator
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('world_generation')

local WorldGenerationService = class()

function WorldGenerationService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._initialized = true
   else
      -- TODO: support tile generation after load
      -- TODO: make sure all rngs dependent on the tile seed
   end
end

function WorldGenerationService:create_new_game(seed, async)
   self:set_seed(seed)
   self._async = async
   self._enable_scenarios = radiant.util.get_config('enable_scenarios', true)

   self._terrain_info = TerrainInfo()
   self._micro_map_generator = MicroMapGenerator(self._terrain_info, self._rng)
   self._terrain_generator = TerrainGenerator(self._terrain_info, self._rng)
   self._height_map_renderer = HeightMapRenderer(self._terrain_info)
   self._landscaper = Landscaper(self._terrain_info, self._rng)
   self._habitat_manager = HabitatManager(self._terrain_info, self._landscaper)

   self._scenario_index = ScenarioIndex(self._terrain_info, self._rng)
   self._ore_scenario_selector = OreScenarioSelector(self._scenario_index, self._terrain_info, self._rng)
   self._surface_scenario_selector = SurfaceScenarioSelector(self._scenario_index, self._terrain_info, self._rng)
   stonehearth.static_scenario:create_new_game(seed)
   stonehearth.dynamic_scenario:create_new_game()

   self.blueprint_generator = BlueprintGenerator()
   self.overview_map = OverviewMap(self._terrain_info, self._landscaper)

   self._starting_location = nil
end

function WorldGenerationService:get_terrain_info()
   return self._terrain_info
end

function WorldGenerationService:set_seed(seed)
   log:warning('WorldGenerationService using seed %d', seed)
   self._sv.seed = seed
   self.__saved_variables:mark_changed()

   self._rng = RandomNumberGenerator(self._sv.seed)
end

function WorldGenerationService:get_seed()
   return self._sv.seed
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
         local tile_size = self._terrain_info.tile_size
         local macro_blocks_per_tile = tile_size / self._terrain_info.macro_block_size
         local blueprint_generator = self.blueprint_generator
         local micro_map_generator = self._micro_map_generator
         local landscaper = self._landscaper
         local full_micro_map, full_underground_micro_map
         local full_elevation_map, full_underground_elevation_map, full_feature_map, full_habitat_map

         full_micro_map, full_elevation_map = micro_map_generator:generate_micro_map(blueprint)
         full_underground_micro_map, full_underground_elevation_map = micro_map_generator:generate_underground_micro_map(full_micro_map)

         full_feature_map = Array2D(full_elevation_map.width, full_elevation_map.height)

         -- determine which features will be placed in which cells
         landscaper:mark_trees(full_elevation_map, full_feature_map)
         landscaper:mark_berry_bushes(full_elevation_map, full_feature_map)
         landscaper:mark_flowers(full_elevation_map, full_feature_map)

         full_habitat_map = self._habitat_manager:derive_habitat_map(full_elevation_map, full_feature_map)

         -- shard the maps and store in the blueprint
         -- micro_maps are overlapping so they need a different sharding function
         -- these maps are at macro_block_size resolution (32x32)
         blueprint_generator:store_micro_map(blueprint, "micro_map", full_micro_map, macro_blocks_per_tile)
         blueprint_generator:store_micro_map(blueprint, "underground_micro_map", full_underground_micro_map, macro_blocks_per_tile)

         -- these maps are at feature_size resolution (16x16)
         blueprint_generator:shard_and_store_map(blueprint, "elevation_map", full_elevation_map)
         blueprint_generator:shard_and_store_map(blueprint, "underground_elevation_map", full_underground_elevation_map)
         blueprint_generator:shard_and_store_map(blueprint, "feature_map", full_feature_map)
         blueprint_generator:shard_and_store_map(blueprint, "habitat_map", full_habitat_map)

         -- location of the world origin in the coordinate system of the blueprint
         blueprint.origin_x = math.floor(blueprint.width * tile_size / 2)
         blueprint.origin_y = math.floor(blueprint.height * tile_size / 2)

         -- create the overview map
         self.overview_map:derive_overview_map(full_elevation_map, full_feature_map, blueprint.origin_x, blueprint.origin_y)

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

   local exclusion_radius = stonehearth.static_scenario:get_reveal_distance()
   local exclusion_region = Region2(Rect2(
         Point2(-exclusion_radius,  -exclusion_radius),
         Point2( exclusion_radius+1, exclusion_radius+1)
      ))
   exclusion_region:translate(self._starting_location)

   -- clear the starting location of all revealed scenarios
   stonehearth.static_scenario:reveal_region(exclusion_region, function()
         return false
      end)

   if radiant.util.get_config('enable_full_vision', false) then
      local radius = radiant.math.MAX_INT32-1
      local region = Region2(Rect2(
            Point2(-radius,  -radius),
            Point2( radius+1, radius+1)
         ))
      region:translate(self._starting_location)
      stonehearth.static_scenario:reveal_region(region)
   end
end

-- get the (i,j) index of the blueprint tile for the world coordinates (x,y)
function WorldGenerationService:get_tile_index(x, y)
   local blueprint = self._blueprint
   local tile_size = self._terrain_info.tile_size
   local i = math.floor((x + blueprint.origin_x) / tile_size) + 1
   local j = math.floor((y + blueprint.origin_y) / tile_size) + 1
   return i, j
end

-- get the world coordinates of the origin (top-left corner) of the tile
function WorldGenerationService:get_tile_origin(i, j, blueprint)
   local x, y
   local tile_size = self._terrain_info.tile_size

   x = (i-1)*tile_size - blueprint.origin_x
   y = (j-1)*tile_size - blueprint.origin_y

   return x, y
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

               self:_generate_tile_internal(a, b)

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
         self:_generate_tile_internal(i, j)
      end
   )
end

function WorldGenerationService:_generate_tile_internal(i, j)
   local blueprint = self._blueprint
   local tile_size = self._terrain_info.tile_size
   local tile_map, underground_tile_map, tile_info, tile_seed
   local micro_map, underground_micro_map
   local elevation_map, underground_elevation_map, feature_map, habitat_map
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
   underground_micro_map = tile_info.underground_micro_map
   elevation_map = tile_info.elevation_map
   underground_elevation_map = tile_info.underground_elevation_map
   feature_map = tile_info.feature_map
   habitat_map = tile_info.habitat_map

   -- generate the high resolution heightmap for the tile
   local seconds = Timer.measure(
      function()
         tile_map = self._terrain_generator:generate_tile(micro_map)
         underground_tile_map = self._terrain_generator:generate_underground_tile(underground_micro_map)
      end
   )
   log:info('Terrain generation time: %.3fs', seconds)
   self:_yield()

   -- render heightmap to region3
   self:_render_heightmap_to_region3(tile_map, underground_tile_map, feature_map, offset_x, offset_y)
   self:_yield()

   -- place flora
   self:_place_flora(tile_map, feature_map, offset_x, offset_y)
   self:_yield()

   -- place scenarios
   self:_place_scenarios(habitat_map, elevation_map, underground_elevation_map, offset_x, offset_y)
   self:_yield()

   tile_info.generated = true
end

function WorldGenerationService:_render_heightmap_to_region3(tile_map, underground_tile_map, feature_map, offset_x, offset_y)
   local renderer = self._height_map_renderer
   local region3 = Region3()

   local seconds = Timer.measure(
      function()
         renderer:render_height_map_to_region(region3, tile_map, underground_tile_map)
         renderer:add_region_to_terrain(region3, offset_x, offset_y)
      end
   )

   log:info('Height map to region3 time: %.3fs', seconds)
end

function WorldGenerationService:_place_flora(tile_map, feature_map, offset_x, offset_y)
   local seconds = Timer.measure(
      function()
         self._landscaper:place_flora(tile_map, feature_map, offset_x, offset_y)
      end
   )

   log:info('Landscaper time: %.3fs', seconds)
end

function WorldGenerationService:_place_scenarios(habitat_map, elevation_map, underground_elevation_map, offset_x, offset_y)
   if not self._enable_scenarios then
      return
   end

   local seconds = Timer.measure(
      function()
         self._surface_scenario_selector:place_immediate_scenarios(habitat_map, elevation_map, offset_x, offset_y)

         self._ore_scenario_selector:place_revealed_scenarios(underground_elevation_map, elevation_map, offset_x, offset_y)
         self._surface_scenario_selector:place_revealed_scenarios(habitat_map, elevation_map, offset_x, offset_y)
      end
   )

   log:info('Static scenario time: %.3fs', seconds)
end

function WorldGenerationService:_get_tile_seed(x, y)
   local location_hash = Point2(x, y):hash()
   -- using Point2 as an integer pair hash
   local tile_seed = Point2(self._sv.seed, location_hash):hash()
   return tile_seed
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
