local Array2D = require 'services.world_generation.array_2D'
local TerrainType = require 'services.world_generation.terrain_type'
local TerrainGenerator = require 'services.world_generation.terrain_generator'
local Landscaper = require 'services.world_generation.landscaper'
local HeightMapRenderer = require 'services.world_generation.height_map_renderer'
local Timer = require 'services.world_generation.timer'
local Point3 = _radiant.csg.Point3

local WorldGenerator = class()
local log = radiant.log.create_logger('world_generation')

function WorldGenerator:__init(async, seed)
   self._async = async
   self._progress = 0

   local tg = TerrainGenerator(self._async, seed)
   self._terrain_generator = tg
   self._height_map_renderer = HeightMapRenderer(tg.tile_size, tg.terrain_info)
   self._landscaper = Landscaper(tg.terrain_info, tg.tile_size, tg.tile_size)

   radiant.events.listen(radiant.events, 'stonehearth:slow_poll', self, self.on_poll)
end

function WorldGenerator:on_poll()
   radiant.events.trigger(radiant.events, 'stonehearth:generate_world_progress', { 
      progress = self._progress
   })

   if self._progress == 100 then
      radiant.events.unlisten(radiant.events, 'stonehearth:slow_poll', self, self.on_poll)
   end

end

function WorldGenerator:create_world()
   
   local terrain_thread = function()
      local cpu_timer = Timer(Timer.CPU_TIME)
      local wall_clock_timer = Timer(Timer.WALL_CLOCK)
      cpu_timer:start()
      wall_clock_timer:start()

      local tiles
      tiles = self:_create_world_blueprint()
      --tiles = self:_create_test_blueprint()
      self:_generate_world(tiles)

      cpu_timer:stop()
      wall_clock_timer:stop()
      log:info('World generation cpu time (excludes terrain ring tesselator): %.3fs', cpu_timer:seconds())
      log:info('World generation wall clock time: %.0fs', wall_clock_timer:seconds())
   end

   if self._async then
      radiant.create_background_task('Terrain Generator', terrain_thread)     
   else
      terrain_thread()
   end
end

function WorldGenerator:_generate_world(tiles)
   local num_tiles_x = tiles.width
   local num_tiles_y = tiles.height
   local terrain_info = self._terrain_generator.terrain_info
   local tile_size = self._terrain_generator.tile_size
   local renderer = self._height_map_renderer
   local timer = Timer(Timer.CPU_TIME)
   local tile_map, micro_map, tile_info
   local origin_x, origin_y, offset_x, offset_y, offset_pt
   local i, j, n, tile_order_list
   local region3_boxed

   tile_order_list = self:_build_tile_order_list(tiles)

   origin_x = num_tiles_x * tile_size / 2
   origin_y = num_tiles_y * tile_size / 2

   for n=1, #tile_order_list do
      i = tile_order_list[n].x
      j = tile_order_list[n].y 
      tile_info = tiles:get(i, j)
      assert(not tile_info.generated)

      tile_map, micro_map = self._terrain_generator:generate_tile(tile_info.terrain_type, tiles, i, j)
      tiles:set(i, j, micro_map)
      self:_yield()

      offset_x = (i-1)*tile_size-origin_x
      offset_y = (j-1)*tile_size-origin_y
      offset_pt = Point3(offset_x, 0, offset_y)

      timer:start()
      region3_boxed = renderer:create_new_region()
      renderer:render_height_map_to_region(region3_boxed, tile_map)
      self._landscaper:place_boulders(region3_boxed, tile_map)
      renderer:add_region_to_terrain(region3_boxed, offset_pt)
      timer:stop()
      log:info('HeightMapRenderer time: %.3fs', timer:seconds())
      self:_yield()

      timer:start()
      self._landscaper:place_flora(tile_map, offset_x, offset_y)
      timer:stop()
      log:info('Landscaper time: %.3fs', timer:seconds())
      self:_yield()

      self._progress = (n / #tile_order_list) * 100

   end

   radiant.events.trigger(radiant.events, 'stonehearth:generate_world_progress', {
      progress = 1,
      complete = true
   })

end

function WorldGenerator:_yield()
   if self._async then
      coroutine.yield()
   end
end

function WorldGenerator:_create_test_blueprint()
   local tiles = self:_get_empty_blueprint(1, 1, TerrainType.Grassland)

   --tiles:get(1, 1).terrain_type = TerrainType.Mountains
   --tiles:get(2, 1).terrain_type = TerrainType.Grassland
   --tiles:get(1, 2).terrain_type = TerrainType.Grassland
   --tiles:get(2, 2).terrain_type = TerrainType.Grassland

   return tiles
end


function WorldGenerator:_create_world_blueprint()
   
   --local r = Rng()
   local t = {
      TerrainType.Grassland,
      TerrainType.Foothills,
      TerrainType.Mountains,
   }

   local tiles = self:_get_empty_blueprint(5, 5)
   for i = 1, 5 do
      for j = 1, 5 do
         --local random = r:generate_uniform_int(1, #t)
         local random = math.random(1, 1000)
         local type
         if random < 400 then
            type = TerrainType.Grassland
         elseif random < 800 then
            type = TerrainType.Foothills
         else
            type = TerrainType.Mountains
         end
         tiles:get(i, j).terrain_type = type
      end
   end
   return tiles
end

function WorldGenerator:_create_world_blueprint_static()
   local tiles = self:_get_empty_blueprint(5, 5)

   tiles:get(1, 1).terrain_type = TerrainType.Grassland
   tiles:get(2, 1).terrain_type = TerrainType.Grassland
   tiles:get(3, 1).terrain_type = TerrainType.Foothills
   tiles:get(4, 1).terrain_type = TerrainType.Mountains
   tiles:get(5, 1).terrain_type = TerrainType.Mountains

   tiles:get(1, 2).terrain_type = TerrainType.Grassland
   tiles:get(2, 2).terrain_type = TerrainType.Grassland
   tiles:get(3, 2).terrain_type = TerrainType.Grassland
   tiles:get(4, 2).terrain_type = TerrainType.Foothills
   tiles:get(5, 2).terrain_type = TerrainType.Mountains

   tiles:get(1, 3).terrain_type = TerrainType.Foothills
   tiles:get(2, 3).terrain_type = TerrainType.Grassland
   tiles:get(3, 3).terrain_type = TerrainType.Grassland
   tiles:get(4, 3).terrain_type = TerrainType.Grassland
   tiles:get(5, 3).terrain_type = TerrainType.Foothills

   tiles:get(1, 4).terrain_type = TerrainType.Mountains
   tiles:get(2, 4).terrain_type = TerrainType.Foothills
   tiles:get(3, 4).terrain_type = TerrainType.Foothills
   tiles:get(4, 4).terrain_type = TerrainType.Grassland
   tiles:get(5, 4).terrain_type = TerrainType.Grassland

   tiles:get(1, 5).terrain_type = TerrainType.Mountains
   tiles:get(2, 5).terrain_type = TerrainType.Mountains
   tiles:get(3, 5).terrain_type = TerrainType.Mountains
   tiles:get(4, 5).terrain_type = TerrainType.Foothills
   tiles:get(5, 5).terrain_type = TerrainType.Grassland

   return tiles
end

function WorldGenerator:_get_empty_blueprint(width, height, terrain_type)
   if terrain_type == nil then terrain_type = TerrainType.Grassland end

   local tiles = Array2D(width, height)
   local i, j, tile_info

   for j=1, tiles.height do
      for i=1, tiles.width do
         tile_info = {}
         tile_info.terrain_type = terrain_type
         tile_info.generated = false
         tiles:set(i, j, tile_info)
      end
   end

   return tiles
end

function WorldGenerator:_build_tile_order_list(map)
   local center_x = (map.width+1)/2
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

         tile_order[#tile_order+1] = coord_info
      end
   end

   local compare_tile = function(a, b)
      return a.dist_metric < b.dist_metric
   end
   table.sort(tile_order, compare_tile)
   return tile_order
end

function WorldGenerator:_get_angle(dy, dx)
   local pi = math.pi

   -- normalize angle to a range of 0 - 2pi
   local value = math.atan2(dy, dx) + pi

   -- move minimum to 45 degrees (pi/4) so fill order looks better
   if value < pi/4 then value = value + 2*pi end
   return value
end

return WorldGenerator

