local build_util = require 'stonehearth.lib.build_util'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

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
   local block_types = radiant.terrain.get_block_types()
   region3:add_cube(Cube3(Point3(-size,  -1, -size), Point3(size, 0, size), block_types.grass))
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
function StonehearthTemplateBuilder:print_walls()
   local bc = self._building:get_component('stonehearth:building')
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
function StonehearthTemplateBuilder:get_wall_with_bounds(bounds_str)
   local bc = self._building:get_component('stonehearth:building')
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

function StonehearthTemplateBuilder:get_offset_on_wall(wall, dx, dy)
   local t = wall:get_component('stonehearth:wall')
                        :get_tangent_coord()
   local location = Point3(radiant.entities.get_location_aligned(wall))
   location[t] = location[t] + dx
   location.y = location.y + dy

   return location
end

-- sticks a portal (door or window) on the wall with `bounds_str` at `location`
--
function StonehearthTemplateBuilder:place_portal_on_wall(bounds_str, portal_uri, dx, dy)
   local wall = self:get_wall_with_bounds(bounds_str)
   local location = self:get_offset_on_wall(wall, dx, dy)
   stonehearth.build:add_fixture(wall, portal_uri, location)
end

-- sticks a fixture (lamp, curtains, etc.) on the wall with `bounds_str` at `location`
-- `normal` can be used to determine which side of the wall the fixture is placed on.
-- if omitted, we put it outside.
--
function StonehearthTemplateBuilder:place_item_on_wall(bounds_str, item_uri, dx, dy, normal)
   local wall = self:get_wall_with_bounds(bounds_str)
   if not normal then
      normal = wall:get_component('stonehearth:construction_data')
                     :get_normal()
   end
   local location = self:get_offset_on_wall(wall, dx, dy)

   stonehearth.build:add_fixture(wall, item_uri, location + normal, normal)
end

function StonehearthTemplateBuilder:place_item_on_floor(item_uri, dx, dz, rotation)
   local bc = self._building:get_component('stonehearth:building')
   local structures = bc:get_all_structures()
   for id, entry in pairs(structures['stonehearth:floor']) do
      local floor = entry.entity
      stonehearth.build:add_fixture(floor, item_uri, Point3(dx, 0, dz), Point3.unit_y, rotation)
      break
   end
end

function StonehearthTemplateBuilder:add_floor(x0, y0, x1, y1, pattern)
   local p0 = Point3(x0, -1, y0)
   local p1 = Point3(x1,  0, y1)
   local floor = stonehearth.build:add_floor(self._session, WOODEN_FLOOR, Cube3(p0, p1), pattern)
   self._building = build_util.get_building_for(floor)
end

function StonehearthTemplateBuilder:grow_walls(column_uri, wall_uri)
   stonehearth.build:grow_walls(self._building, column_uri, wall_uri)
end

function StonehearthTemplateBuilder:grow_roof(roof_uri, options)
   stonehearth.build:grow_roof(self._building, roof_uri, options)
end

-- builds a small house
--
function StonehearthTemplateBuilder:_build_small_house()
   self:add_floor(0, 0, 7, 7, WOODEN_FLOOR_LIGHT)      
   self:grow_walls(WOODEN_COLUMN, WOODEN_WALL)
   self:grow_roof(WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right' },
         nine_grid_max_height = 10,
      })

   self:print_walls()
   self:place_portal_on_wall('((0, 0, 0) - (7, 10, 1))', 'stonehearth:portals:wooden_door', 3, 0)
   self:place_item_on_wall('((0, 0, 0) - (7, 10, 1))', 'stonehearth:furniture:wooden_wall_lantern', 3, 6)
   self:place_item_on_floor('stonehearth:furniture:comfy_bed', 2, 2, 90)
end

function StonehearthTemplateBuilder:_create_template(o)
   radiant.log.write('', 0, 'creating template for "%s"', o.name)
   stonehearth.build:do_command('', nil, function()
         assert(not self._building)
         o.fn(self)
         assert(self._building)
         o.building = self._building
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

   if self._building then
      radiant.entities.destroy_entity(self._building)
      self._building = nil
   end

   local entry = self._templates[self._offset]
   self._offset = self._offset + 1

   self:_create_template(entry)

   return entry
end


stonehearth_templates = {}

radiant.events.listen(stonehearth_templates, 'radiant:new_game', function(args)
      -- create the builder object.  the client will run the show from here on out.
      stonehearth_templates.builder = StonehearthTemplateBuilder()
   end)

return stonehearth_templates
