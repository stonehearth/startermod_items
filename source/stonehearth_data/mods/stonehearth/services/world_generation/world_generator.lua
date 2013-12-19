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
   self._height_map_renderer = HeightMapRenderer(tg.zone_size, tg.terrain_info)
   self._landscaper = Landscaper(tg.terrain_info, tg.zone_size, tg.zone_size)

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

      local zones
      zones = self:_create_world_blueprint()
      --zones = self:_create_test_blueprint()
      self:_generate_world(zones)

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

function WorldGenerator:_generate_world(zones)
   local num_zones_x = zones.width
   local num_zones_y = zones.height
   local terrain_info = self._terrain_generator.terrain_info
   local zone_size = self._terrain_generator.zone_size
   local renderer = self._height_map_renderer
   local timer = Timer(Timer.CPU_TIME)
   local zone_map, micro_map, zone_info
   local origin_x, origin_y, offset_x, offset_y, offset_pt
   local i, j, n, zone_order_list
   local region3_boxed

   zone_order_list = self:_build_zone_order_list(zones)

   origin_x = num_zones_x * zone_size / 2
   origin_y = num_zones_y * zone_size / 2

   for n=1, #zone_order_list do
      i = zone_order_list[n].x
      j = zone_order_list[n].y 
      zone_info = zones:get(i, j)
      assert(not zone_info.generated)

      zone_map, micro_map = self._terrain_generator:generate_zone(zone_info.terrain_type, zones, i, j)
      zones:set(i, j, micro_map)
      self:_yield()

      offset_x = (i-1)*zone_size-origin_x
      offset_y = (j-1)*zone_size-origin_y
      offset_pt = Point3(offset_x, 0, offset_y)

      timer:start()
      region3_boxed = renderer:create_new_region()
      renderer:render_height_map_to_region(region3_boxed, zone_map)
      self._landscaper:place_boulders(region3_boxed, zone_map)
      renderer:add_region_to_terrain(region3_boxed, offset_pt)
      timer:stop()
      log:info('HeightMapRenderer time: %.3fs', timer:seconds())
      self:_yield()

      timer:start()
      self._landscaper:place_flora(zone_map, offset_x, offset_y)
      timer:stop()
      log:info('Landscaper time: %.3fs', timer:seconds())
      self:_yield()

      self._progress = (n / #zone_order_list) * 100

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
   local zones = self:_get_empty_blueprint(1, 1, TerrainType.Grassland)

   --zones:get(1, 1).terrain_type = TerrainType.Mountains
   --zones:get(2, 1).terrain_type = TerrainType.Grassland
   --zones:get(1, 2).terrain_type = TerrainType.Grassland
   --zones:get(2, 2).terrain_type = TerrainType.Grassland

   return zones
end


function WorldGenerator:_create_world_blueprint()
   
   --local r = Rng()
   local t = {
      TerrainType.Grassland,
      TerrainType.Foothills,
      TerrainType.Mountains,
   }

   local zones = self:_get_empty_blueprint(5, 5)
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
         zones:get(i, j).terrain_type = type
      end
   end
   return zones
end

function WorldGenerator:_create_world_blueprint_static()
   local zones = self:_get_empty_blueprint(5, 5)

   zones:get(1, 1).terrain_type = TerrainType.Grassland
   zones:get(2, 1).terrain_type = TerrainType.Grassland
   zones:get(3, 1).terrain_type = TerrainType.Foothills
   zones:get(4, 1).terrain_type = TerrainType.Mountains
   zones:get(5, 1).terrain_type = TerrainType.Mountains

   zones:get(1, 2).terrain_type = TerrainType.Grassland
   zones:get(2, 2).terrain_type = TerrainType.Grassland
   zones:get(3, 2).terrain_type = TerrainType.Grassland
   zones:get(4, 2).terrain_type = TerrainType.Foothills
   zones:get(5, 2).terrain_type = TerrainType.Mountains

   zones:get(1, 3).terrain_type = TerrainType.Foothills
   zones:get(2, 3).terrain_type = TerrainType.Grassland
   zones:get(3, 3).terrain_type = TerrainType.Grassland
   zones:get(4, 3).terrain_type = TerrainType.Grassland
   zones:get(5, 3).terrain_type = TerrainType.Foothills

   zones:get(1, 4).terrain_type = TerrainType.Mountains
   zones:get(2, 4).terrain_type = TerrainType.Foothills
   zones:get(3, 4).terrain_type = TerrainType.Foothills
   zones:get(4, 4).terrain_type = TerrainType.Grassland
   zones:get(5, 4).terrain_type = TerrainType.Grassland

   zones:get(1, 5).terrain_type = TerrainType.Mountains
   zones:get(2, 5).terrain_type = TerrainType.Mountains
   zones:get(3, 5).terrain_type = TerrainType.Mountains
   zones:get(4, 5).terrain_type = TerrainType.Foothills
   zones:get(5, 5).terrain_type = TerrainType.Grassland

   return zones
end

function WorldGenerator:_get_empty_blueprint(width, height, terrain_type)
   if terrain_type == nil then terrain_type = TerrainType.Grassland end

   local zones = Array2D(width, height)
   local i, j, zone_info

   for j=1, zones.height do
      for i=1, zones.width do
         zone_info = {}
         zone_info.terrain_type = terrain_type
         zone_info.generated = false
         zones:set(i, j, zone_info)
      end
   end

   return zones
end

function WorldGenerator:_build_zone_order_list(map)
   local center_x = (map.width+1)/2
   local center_y = (map.height+1)/2
   local zone_order = {}
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

         zone_order[#zone_order+1] = coord_info
      end
   end

   local compare_zone = function(a, b)
      return a.dist_metric < b.dist_metric
   end
   table.sort(zone_order, compare_zone)
   return zone_order
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

