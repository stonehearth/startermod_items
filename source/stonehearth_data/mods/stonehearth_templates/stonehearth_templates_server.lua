local build_util = require 'stonehearth.lib.build_util'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Terrain = _radiant.om.Terrain

local StonehearthTemplateBuilder = class()

local WOODEN_FLOOR = 'stonehearth:entities:wooden_floor'
local WOODEN_FLOOR_DARK = '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_dark.qb'
local WOODEN_FLOOR_LIGHT = '/stonehearth/entities/build/wooden_floor/wooden_floor_solid_light.qb'
local WOODEN_FLOOR_DIAGONAL = '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb'
local WOODEN_COLUMN = 'stonehearth:wooden_column'
local WOODEN_WALL = 'stonehearth:wooden_wall'
local WOODEN_ROOF = 'stonehearth:wooden_peaked_roof'

function StonehearthTemplateBuilder:__init()
   local size = 128
   local region3 = Region3()

   -- create the most boring world possible...
   region3:add_cube(Cube3(Point3(-size,  -1, -size), Point3(size, 0, size), Terrain.GRASS))
   radiant._root_entity:add_component('terrain')
                           :add_tile(region3)

   -- this is the list of templates to create.
   self._templates = {
      { name = 'Small House',  fn = self._build_small_house },
   }
   self._offset = 1
end

-- print the bounds of all the walls in a build.  useful for getting a bounds_str to
-- pass to the helper builder functions
--
local function print_walls(building)
   local bc = building:get_component('stonehearth:building')
   local structures = bc:get_all_structures()
   for id, entry in pairs(structures['stonehearth:wall']) do
      local entity = entry.entity
      local bounds = entity:get_component('destination')
                              :get_region()
                                 :get()
                                    :get_bounds()
      local normal = entity:get_component('stonehearth:construction_data')
                              :get_normal()
      radiant.log.write('', 0, 'wall bounds: %s  normal: %s', bounds, normal)
   end
end

-- get the wall in `building` which matches the `bounds_str` the bounds str is the
-- string representation of the bounds of the wall.  This looks nicer in the source
-- that some bounds constructor monstrosity.  Use `print_walls` if you're trying to
-- find the bounds_str for a particular wall in the building/
--
local function get_wall_with_bounds(building, bounds_str)
   local bc = building:get_component('stonehearth:building')
   local structures = bc:get_all_structures()
   for id, entry in pairs(structures['stonehearth:wall']) do
      local entity = entry.entity
      local bounds = entity:get_component('destination')
                              :get_region()
                                 :get()
                                    :get_bounds()
      if tostring(bounds) == bounds_str then
         return entity
      end
   end
   error(string.format('could not find wall with bounds %s', bounds))
end

-- sticks a portal (door or window) on the wall with `bounds_str` at `location`
--
local function place_portal_on_wall(building, bounds_str, portal_uri, location)
   local wall = get_wall_with_bounds(building, bounds_str)
   stonehearth.build:add_fixture(wall, portal_uri, location)
end

-- sticks a fixture (lamp, curtains, etc.) on the wall with `bounds_str` at `location`
-- `normal` can be used to determine which side of the wall the fixture is placed on.
-- if omitted, we put it outside.
--
local function place_fixture_on_wall(building, bounds_str, fixture_uri, location, normal)
   local wall = get_wall_with_bounds(building, bounds_str)
   if not normal then
      normal = wall:get_component('stonehearth:construction_data')
                     :get_normal()
   end
   stonehearth.build:add_fixture(wall, fixture_uri, location + normal, normal)
end

-- builds a small house
--
function StonehearthTemplateBuilder:_build_small_house()
   local floor = stonehearth.build:add_floor(self._session, WOODEN_FLOOR, Cube3(Point3.zero, Point3(7, 1, 7)), WOODEN_FLOOR_LIGHT)
   local building = build_util.get_building_for(floor)
   stonehearth.build:grow_walls(building, WOODEN_COLUMN, WOODEN_WALL)
   stonehearth.build:grow_roof(building, WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right' },
         nine_grid_max_height = 10,
      })

   print_walls(building)
   place_portal_on_wall(building,  '((0, 0, 0) - (7, 10, 1))', 'stonehearth:portals:wooden_door', Point3(3, 0, 0))
   place_fixture_on_wall(building, '((0, 0, 0) - (7, 10, 1))', 'stonehearth:furniture:wooden_wall_lantern', Point3(3, 6, 0))

   return building
end

function StonehearthTemplateBuilder:_create_template(o)
   radiant.log.write('', 0, 'creating template for "%s"', o.name)
   stonehearth.build:do_command('', nil, function()
         o.building = o.fn(self)
      end)

   stonehearth.build:instabuild(o.building)
   radiant.log.write('', 0, 'finished creating template for "%s"', o.name)
end

function StonehearthTemplateBuilder:build_next_template(session, response)
   if not self._session then
      self._session = session
      stonehearth.town:add_town(session)
      stonehearth.inventory:add_inventory(session)
      stonehearth.population:add_population(session, 'stonehearth:kingdoms:ascendancy')
   end

   if self._offset > #self._templates then
      radiant.log.write('', 0, 'finished processing templates')
      radiant.exit(0)
   end

   if self._last_building then
      radiant.entities.destroy_entity(self._last_building)
      self._last_building = nil
   end

   local entry = self._templates[self._offset]
   self._offset = self._offset + 1

   self:_create_template(entry)
   self._last_building = entry.building

   return entry
end


stonehearth_templates = {}

radiant.events.listen(stonehearth_templates, 'radiant:new_game', function(args)
      -- create the builder object.  the client will run the show from here on out.
      stonehearth_templates.builder = StonehearthTemplateBuilder()
   end)

return stonehearth_templates
