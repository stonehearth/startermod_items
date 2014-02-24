local MathFns = require 'services.world_generation.math.math_fns'
local Timer = require 'services.world_generation.timer'

local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local _terrain = radiant._root_entity:add_component('terrain')
local log = radiant.log.create_logger('visibility')

TerrainService = class()

function TerrainService:__init()
   self._sight_radius = radiant.util.get_config('sight_radius', 64)
   self._visbility_step_size = radiant.util.get_config('visibility_step_size', 8)
   self._visible_regions = {}
   self._explored_regions = {}
   self._last_optimized_rect_count = 10

   self:_register_events()
end

function TerrainService:_register_events()
   radiant.events.listen(radiant.events, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function TerrainService:_on_poll()
   self:_update_regions()
end

function TerrainService:_update_regions()
   local old_visible_region, new_visible_region
   local explored_region_boxed, explored_region, unexplored_region

   for faction_name, visible_region_boxed in pairs(self._visible_regions) do
      explored_region_boxed = self:get_explored_region(faction_name)

      old_visible_region = visible_region_boxed:get()
      new_visible_region = self:_get_visible_region(faction_name)

      if not self:_are_equivalent_regions(old_visible_region, new_visible_region) then
         visible_region_boxed:modify(
            function (region2)
               region2:clear()
               region2:add_region(new_visible_region)
               --log:info('Server visibility rects: %d', region2:get_num_rects())
            end
         )

         explored_region = explored_region_boxed:get()
         unexplored_region = new_visible_region - explored_region

         if not unexplored_region:empty() then
            explored_region_boxed:modify(
               function (region2)
                  region2:add_unique_region(unexplored_region)

                  local num_rects = region2:get_num_rects()
                  if num_rects >= self._last_optimized_rect_count * 1.2 then
                     log:info('Optimizing explored region')

                     local seconds = Timer.measure(
                        function()
                           region2:optimize_by_oct_tree(64)
                        end
                     )
                     log:info('Optimization time: %.3fs', seconds)

                     self._last_optimized_rect_count = region2:get_num_rects()
                  end
               end
            )
         end
      end
   end
end

 -- this will eventually be a non-rectangular region composed of the tiles that have been generated
function TerrainService:_get_terrain_region()
   local region = Region2()
   region:add_cube(_terrain:get_bounds():project_onto_xz_plane())
   return region
end

function TerrainService:_get_visible_region(faction_name)
   local terrain_bounds = self:_get_terrain_region()
   local visible_region = Region2()
   local faction, citizens, entity_region, bounded_visible_region

   -- TODO: where do we get the kingdom name from?
   faction = stonehearth.population:get_faction(faction_name, 'stonehearth:factions:ascendancy')
   citizens = faction:get_citizens()

   for _, entity in pairs(citizens) do
      entity_region = self:_get_entity_visible_region(entity)
      visible_region:add_region(entity_region)
   end

   bounded_visible_region = _radiant.csg.intersect_region2(visible_region, terrain_bounds)

   return bounded_visible_region
end

function TerrainService:_get_entity_visible_region(entity)
   local step_size = self._visbility_step_size
   local quantize = function (value) 
      return MathFns.quantize(value, step_size)
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

   intersection = _radiant.csg.intersect_region2(region_a, region_b)

   return intersection:get_area() == area_a
end

function TerrainService:get_visible_region(faction)
   return self:_get_region(self._visible_regions, faction)
end

function TerrainService:get_explored_region(faction)
   return self:_get_region(self._explored_regions, faction)
end

function TerrainService:_get_region(map, faction)
   local boxed_region = map[faction]

   if not boxed_region then
      boxed_region = _radiant.sim.alloc_region2()
      map[faction] = boxed_region
   end

   return boxed_region
end

return TerrainService()
