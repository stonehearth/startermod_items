local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local _terrain = radiant._root_entity:add_component('terrain')
local log = radiant.log.create_logger('visibility')

TerrainService = class()

function TerrainService:__init()
   self._sight_radius = radiant.util.get_config('sight_radius', 96)
   self._visible_regions = {}
   self._explored_regions = {}

   self:_register_events()
end

function TerrainService:_register_events()
   radiant.events.listen(radiant.events, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function TerrainService:_on_poll()
   local sight_radius = self._sight_radius
   local faction, citizens, old_visible_region, new_visible_region, pt, cube, explored_region_boxed

   local terrain_bounds = _terrain:get_bounds()
   log:info('Terrain bounds: (%d, %d, %d) - (%d, %d, %d)',
      terrain_bounds.min.x, terrain_bounds.min.y, terrain_bounds.min.z,
      terrain_bounds.max.x, terrain_bounds.max.y, terrain_bounds.max.z
   )

   for faction_name, visible_region_boxed in pairs(self._visible_regions) do
      explored_region_boxed = self:get_explored_region(faction_name)

      -- TODO: where do we get the kingdom name from?
      faction = stonehearth.population:get_faction(faction_name, 'stonehearth:factions:ascendancy')

      new_visible_region = Region3()
      citizens = faction:get_citizens()

      for _, entity in pairs(citizens) do
         pt = radiant.entities.get_world_grid_location(entity)

         -- remember +1 on max
         cube = Cube3(Point3(pt.x-sight_radius, pt.y-sight_radius, pt.z-sight_radius),
                      Point3(pt.x+sight_radius+1, pt.y+sight_radius+1, pt.z+sight_radius+1))

         new_visible_region:add_cube(cube)
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
