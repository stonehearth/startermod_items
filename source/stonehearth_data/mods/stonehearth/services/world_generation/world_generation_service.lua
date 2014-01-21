local Array2D = require 'services.world_generation.array_2D'
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
   local event_args = {}

   event_args.progress = self._progress
   if done then
      event_args.width = self.overview_map.width
      event_args.height = self.overview_map.height
      event_args.elevation_map = self.overview_map.elevation_map
      event_args.forest_map = self.overview_map.forest_map
   end

   radiant.events.trigger(radiant.events, 'stonehearth:generate_world_progress', event_args)

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
      blueprint = self:_create_world_blueprint()
      --blueprint = self:_get_empty_blueprint(1, 1) -- useful for debugging real world scenarios without waiting for the load time
      --blueprint = self:_create_world_blueprint_static()
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

   self.overview_map:derive_overview_maps(blueprint)

   radiant.events.trigger(radiant.events, 'stonehearth:generate_world_progress', {
      progress = 1
      --complete = true
   })
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

function WorldGenerationService:_create_world_blueprint()
   local mountains_threshold = 65
   local foothills_threshold = 50
   local num_tiles_x = 5
   local num_tiles_y = 5
   local blueprint = self:_get_empty_blueprint(num_tiles_x, num_tiles_y)
   local noise_map = Array2D(num_tiles_x, num_tiles_y)
   local height_map = Array2D(num_tiles_x, num_tiles_y)
   local i, j, value, terrain_type

   local noise_fn = function(i, j)
      return self._rng:get_gaussian(55, 50)
   end

   while (true) do
      noise_map:fill(noise_fn)
      FilterFns.filter_2D_050(height_map, noise_map, noise_map.width, noise_map.height, 4)

      --log:debug('blueprint noise map')
      --noise_map:print()
      --log:debug('blueprint height map')
      --height_map:print()

      for i=1, num_tiles_y do
         for j=1, num_tiles_x do
            value = height_map:get(i, j)
            if value >= mountains_threshold then
               terrain_type = TerrainType.mountains
            elseif value >= foothills_threshold then
               terrain_type = TerrainType.foothills
            else
               terrain_type = TerrainType.grassland
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

function WorldGenerationService:_is_playable_map(blueprint)
   local i, j, terrain_type, percent_mountains
   local total_tiles = blueprint.width * blueprint.height
   local stats = {}

   stats[TerrainType.grassland] = 0
   stats[TerrainType.foothills] = 0
   stats[TerrainType.mountains] = 0

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         terrain_type = blueprint:get(i, j).terrain_type
         stats[terrain_type] = stats[terrain_type] + 1
      end
   end

   log:debug('Terrain distribution:')
   log:debug('grassland: %d, foothills: %d, mountains: %d', stats[TerrainType.grassland],
      stats[TerrainType.foothills], stats[TerrainType.mountains])

   percent_mountains = stats[TerrainType.mountains] / total_tiles

   return MathFns.in_bounds(percent_mountains, 0.20, 0.40)
end

function WorldGenerationService:_create_world_blueprint_static()
   local blueprint = self:_get_empty_blueprint(5, 5)

   blueprint:get(1, 1).terrain_type = TerrainType.grassland
   blueprint:get(2, 1).terrain_type = TerrainType.grassland
   blueprint:get(3, 1).terrain_type = TerrainType.foothills
   blueprint:get(4, 1).terrain_type = TerrainType.mountains
   blueprint:get(5, 1).terrain_type = TerrainType.mountains

   blueprint:get(1, 2).terrain_type = TerrainType.grassland
   blueprint:get(2, 2).terrain_type = TerrainType.grassland
   blueprint:get(3, 2).terrain_type = TerrainType.grassland
   blueprint:get(4, 2).terrain_type = TerrainType.foothills
   blueprint:get(5, 2).terrain_type = TerrainType.mountains

   blueprint:get(1, 3).terrain_type = TerrainType.foothills
   blueprint:get(2, 3).terrain_type = TerrainType.grassland
   blueprint:get(3, 3).terrain_type = TerrainType.grassland
   blueprint:get(4, 3).terrain_type = TerrainType.grassland
   blueprint:get(5, 3).terrain_type = TerrainType.foothills

   blueprint:get(1, 4).terrain_type = TerrainType.mountains
   blueprint:get(2, 4).terrain_type = TerrainType.foothills
   blueprint:get(3, 4).terrain_type = TerrainType.foothills
   blueprint:get(4, 4).terrain_type = TerrainType.grassland
   blueprint:get(5, 4).terrain_type = TerrainType.grassland

   blueprint:get(1, 5).terrain_type = TerrainType.mountains
   blueprint:get(2, 5).terrain_type = TerrainType.mountains
   blueprint:get(3, 5).terrain_type = TerrainType.mountains
   blueprint:get(4, 5).terrain_type = TerrainType.foothills
   blueprint:get(5, 5).terrain_type = TerrainType.grassland

   return blueprint
end

function WorldGenerationService:_get_empty_blueprint(width, height, terrain_type)
   if terrain_type == nil then terrain_type = TerrainType.grassland end

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
