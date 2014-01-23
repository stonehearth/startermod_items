local Array2D = require 'services.world_generation.array_2D'
local BlueprintGenerator = require 'services.world_generation.blueprint_generator'
local TerrainType = require 'services.world_generation.terrain_type'
local TerrainGenerator = require 'services.world_generation.terrain_generator'
local Landscaper = require 'services.world_generation.landscaper'
local HeightMapRenderer = require 'services.world_generation.height_map_renderer'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local MathFns = require 'services.world_generation.math.math_fns'
local Timer = require 'services.world_generation.timer'
local ScenarioService = require 'services.scenario.scenario_service'
local HabitatType = require 'services.world_generation.habitat_type'
local HabitatManager = require 'services.world_generation.habitat_manager'
local OverviewMap = require 'services.world_generation.overview_map'
local RandomNumberGenerator = _radiant.csg.RandomNumberGenerator
local Point3 = _radiant.csg.Point3

local WorldGenerationService = class()
local log = radiant.log.create_logger('world_generation')

function WorldGenerationService:__init()
end

function WorldGenerationService:initialize(async, game_seed)
   self._async = async
   self._game_seed = game_seed
   self._rng = RandomNumberGenerator(self._game_seed)
   self._progress = 0
   self._enable_scenarios = radiant.util.get_config('enable_scenarios', false)

   log:info('WorldGenerationService using seed %d', self._game_seed)

   self._blueprint_generator = BlueprintGenerator(self._game_seed)

   local tg = TerrainGenerator(self._rng, self._async)
   self._terrain_generator = tg
   self._height_map_renderer = HeightMapRenderer(tg.tile_size, tg.terrain_info)
   self._landscaper = Landscaper(tg.terrain_info, tg.tile_size, tg.tile_size, self._rng, self._async)
   self._terrain_info = tg.terrain_info
   local feature_cell_size = self._landscaper:get_feature_cell_size()
   
   self._scenario_service = radiant.mods.load('stonehearth').scenario
   self._scenario_service:initialize(feature_cell_size, self._rng)
   self._habitat_manager = HabitatManager(self._terrain_info, self._landscaper)

   self.overview_map = OverviewMap(self._terrain_info, self._landscaper,
      tg.tile_size, tg.macro_block_size, feature_cell_size)

   radiant.events.listen(radiant.events, 'stonehearth:slow_poll', self, self._on_poll_progress)
end

function WorldGenerationService:_on_poll_progress()
   local done = self._progress == 100

   radiant.events.trigger(radiant.events, 'stonehearth:generate_world_progress', {
      progress = self._progress
   })

   if done then
      radiant.events.unlisten(radiant.events, 'stonehearth:slow_poll', self, self._on_poll_progress)
   end
end

function WorldGenerationService:create_world()
   local terrain_thread = function()
      local cpu_timer = Timer(Timer.CPU_TIME)
      local wall_clock_timer = Timer(Timer.WALL_CLOCK)
      cpu_timer:start()
      wall_clock_timer:start()

      local blueprint
      blueprint = self._blueprint_generator:generate_blueprint(5, 5)
      --blueprint = blueprint = self._blueprint_generator:get_empty_blueprint(1, 1) -- useful for debugging real world scenarios without waiting for the load time
      --blueprint = blueprint = self._blueprint_generator:get_static_blueprint()
      self:_generate_world(blueprint)

      cpu_timer:stop()
      wall_clock_timer:stop()
      log:info('World generation cpu time (excludes terrain ring tesselator): %.3fs', cpu_timer:seconds())
      log:info('World generation wall clock time: %.0fs', wall_clock_timer:seconds())
   end

   self.overview_map:clear()

   if self._async then
      radiant.create_background_task('Terrain Generator', terrain_thread)     
   else
      terrain_thread()
   end
end

function WorldGenerationService:_generate_world(blueprint)
   local num_tiles_x = blueprint.width
   local num_tiles_y = blueprint.height
   local terrain_info = self._terrain_generator.terrain_info
   local tile_size = self._terrain_generator.tile_size
   local tile_map, micro_map, tile_info, tile_seed
   local origin_x, origin_y, offset_x, offset_y, offset
   local i, j, n, tile_order_list, num_tiles

   tile_order_list = self:_build_tile_order_list(blueprint)
   num_tiles = #tile_order_list

   -- location of the world origin in the coordinate system of the blueprint
   origin_x = math.floor(num_tiles_x * tile_size / 2)
   origin_y = math.floor(num_tiles_y * tile_size / 2)

   for n=1, num_tiles do
      i = tile_order_list[n].x
      j = tile_order_list[n].y 
      tile_info = blueprint:get(i, j)
      assert(not tile_info.generated)

      -- calculate the world offset of the tile
      offset_x = (i-1)*tile_size-origin_x
      offset_y = (j-1)*tile_size-origin_y

      -- convert to world coordinate system
      offset = Point3(offset_x, 0, offset_y)
      tile_info.offset = offset

      -- make each tile deterministic on its coordinates (and game seed)
      tile_seed = self:_get_tile_seed(i, j)
      self._rng:set_seed(tile_seed)

      -- generate the heightmap for the tile
      tile_map, micro_map = self._terrain_generator:generate_tile(tile_info.terrain_type, blueprint, i, j)
      tile_info.micro_map = micro_map
      self:_yield()

      -- clear features for new tile
      self._landscaper:clear_feature_map()

      -- render heightmap to region3
      self:_render_heightmap_to_region3(tile_map, offset)

      -- place flora
      self:_place_flora(tile_map, offset)

      tile_info.feature_map = self._landscaper:get_feature_map()

      -- derive habitat maps
      tile_info.habitat_map, tile_info.elevation_map =
         self._habitat_manager:derive_habitat_map(tile_info.feature_map, micro_map)

      -- place initial scenarios
      self:_place_scenarios(tile_info.habitat_map, tile_info.elevation_map, offset)

      tile_info.generated = true

      -- update progress bar
      self._progress = (n / num_tiles) * 100
   end

   self.overview_map:derive_overview_map(blueprint, origin_x, origin_y)

   -- trigger finished without waiting for the next poll
   self._on_poll_progress()
end

function WorldGenerationService:_render_heightmap_to_region3(tile_map, offset)
   local renderer = self._height_map_renderer
   local region3_boxed
   local timer = Timer(Timer.CPU_TIME)
   timer:start()

   region3_boxed = renderer:create_new_region()
   renderer:render_height_map_to_region(region3_boxed, tile_map)
   self._landscaper:place_boulders(region3_boxed, tile_map)
   renderer:add_region_to_terrain(region3_boxed, offset)

   timer:stop()
   log:info('HeightMapRenderer time: %.3fs', timer:seconds())
   self:_yield()
end

function WorldGenerationService:_place_flora(tile_map, offset)
   local timer = Timer(Timer.CPU_TIME)
   timer:start()

   self._landscaper:place_flora(tile_map, offset.x, offset.z)

   timer:stop()
   log:info('Landscaper time: %.3fs', timer:seconds())
   self:_yield()
end

function WorldGenerationService:_place_scenarios(habitat_map, elevation_map, offset)
   if not self._enable_scenarios then return end

   local timer = Timer(Timer.CPU_TIME)
   timer:start()

   self._scenario_service:place_scenarios(habitat_map, elevation_map, offset.x, offset.z)

   timer:stop()
   log:info('ScenarioService time: %.3fs', timer:seconds())
   self:_yield()
end

function WorldGenerationService:_get_tile_seed(x, y)
   local tile_hash = MathFns.point_hash(x, y)
   return (self._game_seed + tile_hash) % MathFns.MAX_UINT32
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

function WorldGenerationService:_yield()
   if self._async then
      coroutine.yield()
   end
end

return WorldGenerationService
