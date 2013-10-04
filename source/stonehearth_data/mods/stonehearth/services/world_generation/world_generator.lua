local Array2D = require 'services.world_generation.array_2D'
local TerrainType = require 'services.world_generation.terrain_type'
local TerrainGenerator = require 'services.world_generation.terrain_generator'
local Landscaper = require 'services.world_generation.landscaper'
local HeightMapRenderer = require 'services.world_generation.height_map_renderer'
local Timer = radiant.mods.require('stonehearth_debugtools.timer')

local WorldGenerator = class()

function WorldGenerator:__init()
   self._terrain_generator = TerrainGenerator()
   self._height_map_renderer = HeightMapRenderer(self._terrain_generator.zone_size)
   self._landscaper = Landscaper(self._terrain_generator.terrain_info)
end

function WorldGenerator:create_world(use_async)
   self._use_async = use_async
   
   local terrain_thread = function()
      self._terrain_generator:set_async(self._use_async)
      local cpu_timer = Timer(Timer.CPU_TIME)
      local wall_clock_timer = Timer(Timer.WALL_CLOCK)
      cpu_timer:start()
      wall_clock_timer:start()

      local zones = self:_create_world_blueprint()
      self:_generate_world(zones)

      self._terrain_generator:set_async(false)
      cpu_timer:stop()
      wall_clock_timer:stop()
      radiant.log.info('World generation cpu time (excludes terrain ring tesselator): %.3fs', cpu_timer:seconds())
      radiant.log.info('World generation wall clock time: %.0fs', wall_clock_timer:seconds())
   end

   if self._use_async then
      radiant.create_background_task('generating terrain', terrain_thread)     
   else
      terrain_thread()
   end
end

function WorldGenerator:_generate_world(zones)
   local num_zones_x = zones.width
   local num_zones_y = zones.height
   local terrain_info = self._terrain_generator.terrain_info
   local zone_size = self._terrain_generator.zone_size
   local timer = Timer(Timer.CPU_TIME)
   local zone_map, micro_map, zone_info
   local origin_x, origin_y, offset_x, offset_y
   local i, j, n, zone_order_list

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

      offset_x = (i-1)*zone_size - origin_x
      offset_y = (j-1)*zone_size - origin_y

      timer:start()
      self._height_map_renderer:render_height_map_to_terrain(zone_map, terrain_info, offset_x, offset_y)
      timer:stop()
      radiant.log.info('HeightMapRenderer time: %.3fs', timer:seconds())

      timer:start()
      self._landscaper:place_trees(zone_map, offset_x, offset_y)
      timer:stop()
      radiant.log.info('Landscaper time: %.3fs', timer:seconds())
   end
end

-- do this programmatically later
function WorldGenerator:_create_world_blueprint()
   local zones = Array2D(5, 5)
   local zone_info
   local i, j

   for j=1, zones.height do
      for i=1, zones.width do
         zone_info = {}
         zone_info.terrain_type = TerrainType.Plains
         zone_info.generated = false
         zones:set(i, j, zone_info)
      end
   end

   -- zones:get(1, 1).terrain_type = TerrainType.Mountains
   -- zones:get(2, 1).terrain_type = TerrainType.Plains

   zones:get(1, 1).terrain_type = TerrainType.Mountains
   zones:get(2, 1).terrain_type = TerrainType.Mountains
   zones:get(3, 1).terrain_type = TerrainType.Mountains
   zones:get(4, 1).terrain_type = TerrainType.Foothills
   zones:get(5, 1).terrain_type = TerrainType.Plains

   zones:get(1, 2).terrain_type = TerrainType.Mountains
   zones:get(2, 2).terrain_type = TerrainType.Mountains
   zones:get(3, 2).terrain_type = TerrainType.Foothills
   zones:get(4, 1).terrain_type = TerrainType.Plains
   zones:get(5, 1).terrain_type = TerrainType.Plains

   zones:get(1, 3).terrain_type = TerrainType.Mountains
   zones:get(2, 3).terrain_type = TerrainType.Foothills
   zones:get(3, 3).terrain_type = TerrainType.Plains
   zones:get(4, 3).terrain_type = TerrainType.Plains
   zones:get(5, 3).terrain_type = TerrainType.Foothills

   zones:get(1, 4).terrain_type = TerrainType.Foothills
   zones:get(2, 4).terrain_type = TerrainType.Plains
   zones:get(3, 4).terrain_type = TerrainType.Plains
   zones:get(4, 4).terrain_type = TerrainType.Foothills
   zones:get(5, 4).terrain_type = TerrainType.Mountains

   zones:get(1, 5).terrain_type = TerrainType.Plains
   zones:get(2, 5).terrain_type = TerrainType.Plains
   zones:get(3, 5).terrain_type = TerrainType.Plains
   zones:get(4, 5).terrain_type = TerrainType.Foothills
   zones:get(5, 5).terrain_type = TerrainType.Mountains

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

