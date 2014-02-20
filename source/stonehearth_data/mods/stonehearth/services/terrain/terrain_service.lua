local MathFns = require 'services.world_generation.math.math_fns'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local _terrain = radiant._root_entity:add_component('terrain')
local log = radiant.log.create_logger('visibility')

TerrainService = class()

function TerrainService:__init()
   self._sight_radius = radiant.util.get_config('sight_radius', 64)
   self._visbility_step_size = radiant.util.get_config('visibility_step_size', 8)
   self._visible_regions = {}
   self._explored_regions = {}

   self:_register_events()
end

function TerrainService:_register_events()
   radiant.events.listen(radiant.events, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function TerrainService:_on_poll()
   local terrain_bounds = self:_get_terrain_region()
   local old_visible_region, new_visible_region, explored_region_boxed
   local faction, citizens, pt, entity_region, bounded_entity_region

   for faction_name, visible_region_boxed in pairs(self._visible_regions) do
      explored_region_boxed = self:get_explored_region(faction_name)

      -- TODO: where do we get the kingdom name from?
      faction = stonehearth.population:get_faction(faction_name, 'stonehearth:factions:ascendancy')

      new_visible_region = Region3()
      citizens = faction:get_citizens()

      for _, entity in pairs(citizens) do
         entity_region = self:_get_entity_visible_region(entity)
         bounded_entity_region = _radiant.csg.intersect_region3(entity_region, terrain_bounds)
         new_visible_region:add_region(bounded_entity_region)
      end

      old_visible_region = visible_region_boxed:get()

      if not self:_are_equivalent_regions(old_visible_region, new_visible_region) then
         visible_region_boxed:modify(
            function (region3)
               region3:clear()
               region3:add_region(new_visible_region)
               log:info('Server visibility cubes: %d', region3:get_num_rects())
            end
         )

         explored_region_boxed:modify(
            function (region3)
               region3:add_region(new_visible_region)
               log:info('Server explored cubes: %d', region3:get_num_rects())
            end
         )
      else
         log:info('Server visibility has not changed.')
      end
   end
end

 -- this will eventually be a non-rectangular region composed of the tiles that have been generated
function TerrainService:_get_terrain_region()
   local region = Region3()
   region:add_cube(_terrain:get_bounds())
   return region
end

function TerrainService:_get_entity_visible_region(entity)
   local step_size = self._visbility_step_size
   local quantize = function (value) 
      return MathFns.quantize(value, step_size)
   end
   -- fix y bounds until renderer supports 3d bounds. minimizes cubes for now
   local y_min = quantize(0)
   local y_max = quantize(200)
   local region = Region3()
   local semi_major_axis, semi_minor_axis, pt, cube

   semi_major_axis = self._sight_radius
   -- quantize delta to make sure the major and minor axes reveal at the same time
   semi_minor_axis = semi_major_axis - quantize(semi_major_axis * 0.4)

   pt = radiant.entities.get_world_grid_location(entity)

   -- remember +1 on max
   cube = Cube3(
      Point3(quantize(pt.x-semi_major_axis),   y_min, quantize(pt.z-semi_minor_axis)),
      Point3(quantize(pt.x+semi_major_axis+1), y_max, quantize(pt.z+semi_minor_axis+1))
   )

   region:add_cube(cube)

   cube = Cube3(
      Point3(quantize(pt.x-semi_minor_axis),   y_min, quantize(pt.z-semi_major_axis)),
      Point3(quantize(pt.x+semi_minor_axis+1), y_max, quantize(pt.z+semi_major_axis+1))
   )

   region:add_cube(cube)

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

   intersection = _radiant.csg.intersect_region3(region_a, region_b)

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
      boxed_region = _radiant.sim.alloc_region()
      map[faction] = boxed_region
   end

   return boxed_region
end

return TerrainService()
