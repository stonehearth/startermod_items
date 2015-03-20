local Timer = require 'services.server.world_generation.timer'

local Point2 = _radiant.csg.Point2
local Point2f = _radiant.csg.Point2f
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local log = radiant.log.create_logger('terrain')

local INFINITE = 1000000

TerrainService = class()

function TerrainService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv._visible_regions then
      self._sv._visible_regions = {}
      self._sv._explored_regions = {}
      self._sv._convex_hull = {}
   end

   self._terrain_component = radiant._root_entity:add_component('terrain')

   -- the radius of the sight sensor in the json files should match this value
   self._sight_radius = radiant.util.get_config('sight_radius', 64)
   self._visbility_step_size = radiant.util.get_config('visibility_step_size', 8)
   self._last_optimized_rect_count = 10
   self._region_optimization_threshold = radiant.util.get_config('region_optimization_threshold', 1.2)

   self._enable_full_vision = radiant.util.get_config('enable_full_vision', false)

   self:_register_events()
end

function TerrainService:_register_events()
   -- fires about once per second at game speed 1
   stonehearth.calendar:set_interval(110, function()
         self:_on_tick()
      end)
end

function TerrainService:_on_tick()
   self:_update_regions()
   self:_update_convex_hull()
   self:_update_terrain_counts()
end

-- Jarvis 'Gift-Wrapping' Algorithm
function TerrainService:_update_convex_hull()
   -- Get the new points from the citizens.
   local new_points = {}
   local friendly_pops = stonehearth.population:get_friendly_populations('player_1')
   for player_id, pop in pairs(friendly_pops) do
      local citizens = pop:get_citizens()
      for _, entity in pairs(citizens) do
         if entity:is_valid() then
            local location = radiant.entities.get_world_grid_location(entity)
            if location then
               table.insert(new_points, location)
            end
         end
      end
   end

   if #new_points < 1 then
      log:info('no points while updating convex hull.')
      return
   end

   -- Look for a new left-most point from the new points
   local new_hull_point = #self._sv._convex_hull > 0 and self._sv._convex_hull[1] or new_points[1]

   -- If we find that point, remove it from the set of new points.
   for _, v in pairs(new_points) do
      if v.x < new_hull_point.x then
         new_hull_point = v
      end
   end

   for i, v in pairs(self._sv._convex_hull) do
      table.insert(new_points, v)
   end

   -- Finally, build the hull
   local point_used_map = {}
   local new_hull = {}
   repeat
      table.insert(new_hull, new_hull_point)
      local endpoint = new_points[1]
      local used_idx = 1
      for j, v in pairs(new_points) do
         if not point_used_map[j] then
            if (endpoint == new_hull_point) or self:_is_left_of(new_hull_point, endpoint, v) or self:_closer_colinear(new_hull_point, endpoint, v) then
               endpoint = v
               used_idx = j
            end
         end
      end
      point_used_map[used_idx] = true
      new_hull_point = endpoint
   until endpoint == new_hull[1]

   log:info('convex hull has %d points', #new_hull)
   self._sv._convex_hull = new_hull
end

function TerrainService:_closer_colinear(start_point, end_point, test_point)
   local line_dir_i = end_point - start_point
   local test_dir_i = test_point - end_point

   local line_dir = Point3(line_dir_i.x, line_dir_i.y, line_dir_i.z)
   line_dir:normalize()

   local test_dir = Point3(test_dir_i.x, test_dir_i.y, test_dir_i.z)
   test_dir:normalize()

   return line_dir:dot(test_dir) > 0.99
end

-- Uses determinants (aka the area of the triangle).  Given a consistent winding order,
-- the sign of the area determins whether test_point is to the left or right of the
-- given line.
function TerrainService:_is_left_of(start_point, end_point, test_point)
   local ae = start_point.x * end_point.z
   local bf = end_point.x * test_point.z
   local cd = test_point.x * start_point.z

   local ce = test_point.x * end_point.z
   local bd = end_point.x * start_point.z
   local af = start_point.x * test_point.z

   return ((ae + bf + cd) - (ce + bd + af)) > 0
end

function TerrainService:_update_regions()
   local old_visible_region, new_visible_region
   local explored_region_boxed, explored_region, unexplored_region

   for player_id, visible_region_boxed in pairs(self._sv._visible_regions) do
      explored_region_boxed = self:get_explored_region(player_id)

      old_visible_region = visible_region_boxed:get()
      new_visible_region = self:_get_visible_region(player_id)

      if not self:_are_equivalent_regions(old_visible_region, new_visible_region) then
         visible_region_boxed:modify(
            function (region2)
               region2:clear()
               region2:add_region(new_visible_region)
               log:info('server visibility rects: %d', region2:get_num_rects())
            end
         )

         explored_region = explored_region_boxed:get()
         unexplored_region = new_visible_region - explored_region

         if not unexplored_region:empty() then
            explored_region_boxed:modify(
               function (region2)
                  region2:add_unique_region(unexplored_region)

                  local num_rects = region2:get_num_rects()

                  if num_rects >= self._last_optimized_rect_count * self._region_optimization_threshold then
                     log:debug('Optimizing explored region')

                     local seconds = Timer.measure(
                        function()
                           region2:optimize_by_oct_tree('unexplored region', 64)
                        end
                     )
                     log:debug('Optimization time: %.3fs', seconds)

                     -- performance counters standardize on milliseconds
                     radiant.set_performance_counter('explored_region:last_optimization_time', seconds*1000, "time")

                     self._last_optimized_rect_count = region2:get_num_rects()
                  end

                  radiant.set_performance_counter('explored_region:num_rects', num_rects)
               end
            )
         end
      end
   end
end

 -- this will eventually be a non-rectangular region composed of the tiles that have been generated
function TerrainService:_get_terrain_region()
   local region = Region2()

   if not self._terrain_component:is_empty() then
      local bounds = self._terrain_component:get_bounds():project_onto_xz_plane()
      region:add_cube(bounds)
   end

   return region
end

function TerrainService:_get_visible_region(player_id)
   local terrain_region = self:_get_terrain_region()
   if self._enable_full_vision then
      return terrain_region
   end

   local visible_region = Region2()
   local pop, citizens, entity_region, bounded_visible_region

   local friendly_pops = stonehearth.population:get_friendly_populations(player_id)
   for player_id, pop in pairs(friendly_pops) do
      citizens = pop:get_citizens()
      for _, entity in pairs(citizens) do
         entity_region = self:_get_entity_visible_region(entity)
         visible_region:add_region(entity_region)
      end
   end

   bounded_visible_region = visible_region:intersect_region(terrain_region)

   return bounded_visible_region
end

function TerrainService:get_entities_in_explored_region(player_id, filter_fn)
   local explored_region_boxed = self:get_explored_region(player_id)
   local explored_region = explored_region_boxed:get()

   local entities_in_region = {}
   for rect in explored_region:each_cube() do
      local min = Point3(rect.min.x, -INFINITE, rect.min.y)
      local cube = Cube3(Point3(rect.min.x, -INFINITE, rect.min.y),
                         Point3(rect.max.x,  INFINITE, rect.max.y))
      local entities = radiant.terrain.get_entities_in_cube(cube)
      for id, entity in pairs(entities) do 
         if id ~= 1 and (not filter_fn or filter_fn(entity)) then
            entities_in_region[id] = entity
         end
      end
   end 

   return entities_in_region
end

function TerrainService:_get_entity_visible_region(entity)
   local step_size = self._visbility_step_size
   local quantize = function (value) 
      return radiant.math.quantize(value, step_size)
   end
   -- fix y bounds until renderer supports 3d bounds. minimizes cubes for now
   local y_min = quantize(0)
   local y_max = quantize(200)
   local region = Region2()
   local semi_major_axis, semi_minor_axis, pt, rect

   semi_major_axis = self._sight_radius
   -- quantize delta to make sure the major and minor axes reveal at the same time
   semi_minor_axis = semi_major_axis - quantize(semi_major_axis * 0.4)

   pt = radiant.entities.get_world_grid_location(entity)
      
   if pt then
      -- remember +1 on max
      rect = Rect2(
         Point2(quantize(pt.x-semi_major_axis),   quantize(pt.z-semi_minor_axis)),
         Point2(quantize(pt.x+semi_major_axis+1), quantize(pt.z+semi_minor_axis+1))
      )

      region:add_cube(rect)

      rect = Rect2(
         Point2(quantize(pt.x-semi_minor_axis),   quantize(pt.z-semi_major_axis)),
         Point2(quantize(pt.x+semi_minor_axis+1), quantize(pt.z+semi_major_axis+1))
      )

      region:add_cube(rect)
   end

   return region
end

-- ignores tags on the cubes
function TerrainService:_are_equivalent_regions(region_a, region_b)
   local area_a = region_a:get_area()
   local area_b = region_b:get_area()
   local intersection

   if area_a ~= area_b then
      return false
   end

   intersection = region_a:intersect_region(region_b)

   return intersection:get_area() == area_a
end

function TerrainService:get_visible_region(player_id)
   return self:_get_region(self._sv._visible_regions, player_id)
end

function TerrainService:get_explored_region(player_id)
   return self:_get_region(self._sv._explored_regions, player_id)
end

function TerrainService:get_player_perimeter(player_id)
   return self._sv._convex_hull
end

function TerrainService:_get_region(map, player_id)
   local boxed_region = map[player_id]

   if not boxed_region then
      boxed_region = _radiant.sim.alloc_region2()
      map[player_id] = boxed_region
   end

   return boxed_region
end

function TerrainService:_update_terrain_counts()
   local tiles = self._terrain_component:get_tiles()
   --radiant.set_performance_counter('terrain:num_tiles', tiles:num_tiles())
   --radiant.set_performance_counter('terrain:num_cubes', tiles:num_cubes())
end

return TerrainService
