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
   -- this is the list of templates to create.
   self._templates = {
      { name = 'Tiny Cottage',  fn = self._build_tiny_cottage },
      { name = 'Cottage for Two',  fn = self._build_cottage_for_two },
      { name = 'Shared Sleeping Quarters',  fn = self._build_sleeping_hall },
      { name = 'Dining Hall',  fn = self._build_dining_hall },
   }
   self._offset = 1
end

-- print the bounds of all the walls in a build.  useful for getting a bounds_str to
-- pass to the helper builder functions
--
function StonehearthTemplateBuilder:sort_walls()
   local walls = {}
   local bc = self._building:get_component('stonehearth:building')
   local structures = bc:get_all_structures()
   for id, entry in pairs(structures['stonehearth:wall']) do
      local entity = entry.entity
      table.insert(walls, entry.entity)
   end
   table.sort(walls, function(l, r)
         return radiant.entities.get_location_aligned(l) < radiant.entities.get_location_aligned(r)
      end)
   return walls
end

function StonehearthTemplateBuilder:print_walls()
   local walls = self:sort_walls()
   for i, wall in ipairs(walls) do
      local bounds = wall:get_component('destination')
                              :get_region()
                                 :get()
                                    :get_bounds()
      local normal = wall:get_component('stonehearth:construction_data')
                              :get_normal()
      radiant.log.write('', 0, '%2d) wall bounds: %s  normal: %s', i, bounds, normal)
   end
end

-- get the wall in `building` which matches the `bounds_str` the bounds str is the
-- string representation of the bounds of the wall.  This looks nicer in the source
-- that some bounds constructor monstrosity.  Use `print_walls` if you're trying to
-- find the bounds_str for a particular wall in the building/
--
function StonehearthTemplateBuilder:get_wall_with_bounds(bounds_str)
   -- `bounds_str` is an index into the sorted array of walls or a string
   -- for the bounds (see print_walls)
   if type(bounds_str) == 'number' then
      local walls = self:sort_walls()
      return walls[bounds_str]
   end

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

   local location = Point3(0, 0, 0)
   location[t] = dx
   location.y = dy

   return location
end

-- sticks a portal (door or window) on the wall with `bounds_str` at `location`
--
function StonehearthTemplateBuilder:place_portal_on_wall(bounds_str, portal_uri, dx, dy)
   local wall = self:get_wall_with_bounds(bounds_str)
   local location = self:get_offset_on_wall(wall, dx, dy)
   radiant.log.write('', 0, 'placing portal at %s', location)
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
   radiant.log.write('', 0, "building is %s", self._building)
end

function StonehearthTemplateBuilder:grow_walls(column_uri, wall_uri)
   stonehearth.build:grow_walls(self._building, column_uri, wall_uri)
end

function StonehearthTemplateBuilder:grow_roof(roof_uri, options)
   stonehearth.build:grow_roof(self._building, roof_uri, options)
end

-- builds a small house
--
function StonehearthTemplateBuilder:_build_tiny_cottage()
   self:add_floor(0, 0, 7, 6, WOODEN_FLOOR_DIAGONAL)      
   self:grow_walls(WOODEN_COLUMN, WOODEN_WALL)
   self:grow_roof(WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right' },
         nine_grid_max_height = 10,
      })

   self:print_walls()  
   self:place_portal_on_wall('((0, 0, -5) - (1, 6, 1))', 'stonehearth:portals:wooden_window_frame', -2, 2)      
   self:place_portal_on_wall('((0, 0, 0) - (7, 10, 1))', 'stonehearth:portals:wooden_door', 3, 0)
   self:place_item_on_wall('((0, 0, 0) - (7, 10, 1))', 'stonehearth:furniture:wooden_wall_lantern', 3, 6)
   self:place_item_on_floor('stonehearth:furniture:not_much_of_a_bed', 5, 3, 0)
end

function StonehearthTemplateBuilder:_build_cottage_for_two()
   self:add_floor(0, 0, 11, 6, WOODEN_FLOOR_DIAGONAL)      
   self:add_floor(3, -4, 8, 0, WOODEN_FLOOR_DIAGONAL)      
   self:grow_walls(WOODEN_COLUMN, WOODEN_WALL)
   self:grow_roof(WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right' },
         nine_grid_max_height = 10,
      })

   self:print_walls()  
   self:place_portal_on_wall(5, 'stonehearth:portals:wooden_door', 2, 0)
   self:place_item_on_wall(5, 'stonehearth:furniture:wooden_wall_lantern', 2, 6)
   self:place_portal_on_wall(9, 'stonehearth:portals:wooden_window_frame', 3, 2)
   self:place_portal_on_wall(1, 'stonehearth:portals:wooden_window_frame', -2, 2)

   self:place_item_on_floor('stonehearth:furniture:not_much_of_a_bed', 1, 3, 0)
   self:place_item_on_floor('stonehearth:furniture:not_much_of_a_bed', 9, 3, 0)
end

function StonehearthTemplateBuilder:_build_dining_hall()
   self:add_floor(0, 0, 15, 8, WOODEN_FLOOR_DIAGONAL)
   self:add_floor(3, -2, 12, 0, WOODEN_FLOOR_DIAGONAL)
   self:grow_walls(WOODEN_COLUMN, WOODEN_WALL)
   self:grow_roof(WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right', 'front', 'back' },
         nine_grid_max_height = 4,
      })

   self:print_walls()  


   self:place_portal_on_wall(4, 'stonehearth:portals:wooden_door', 4, 0)
   self:place_item_on_wall(4, 'stonehearth:furniture:wooden_wall_lantern', 7, 2)
   self:place_item_on_wall(4, 'stonehearth:furniture:wooden_wall_lantern', 1, 2)

   self:place_portal_on_wall(8, 'stonehearth:portals:wooden_window_frame', 4, 2)

   self:place_portal_on_wall(1, 'stonehearth:portals:wooden_window_frame', -3, 2)      

   self:place_item_on_floor('stonehearth:furniture:dining_table', 4, 4, 90)
   self:place_item_on_floor('stonehearth:furniture:dining_table', 6, 4, 90)
   self:place_item_on_floor('stonehearth:furniture:dining_table', 8, 4, 90)
   self:place_item_on_floor('stonehearth:furniture:dining_table', 10, 4, 90)

   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 2, 4, -90)
   
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 4, 6, 0)
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 6, 6, 0)
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 8, 6, 0)
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 10, 6, 0)
   
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 4, 2, 180)
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 6, 2, 180)
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 8, 2, 180)
   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 10, 2, 180)

   self:place_item_on_floor('stonehearth:furniture:simple_wooden_chair', 12, 4, 90)

end

function StonehearthTemplateBuilder:_build_sleeping_hall()
   self:add_floor(0, 0, 19, 10, WOODEN_FLOOR_DIAGONAL)
   self:add_floor(3, -2, 12, 0, WOODEN_FLOOR_DIAGONAL)
   self:grow_walls(WOODEN_COLUMN, WOODEN_WALL)
   self:grow_roof(WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right', 'front', 'back' },
         nine_grid_max_height = 4,
      })

   self:print_walls()  

   self:place_portal_on_wall(4, 'stonehearth:portals:wooden_door', 4, 0)
   self:place_item_on_wall(4, 'stonehearth:furniture:wooden_wall_lantern', 7, 2)
   self:place_item_on_wall(4, 'stonehearth:furniture:wooden_wall_lantern', 1, 2)

   self:place_portal_on_wall(6, 'stonehearth:portals:wooden_window_frame', 3, 2)      
   
   self:place_portal_on_wall(8, 'stonehearth:portals:wooden_window_frame', 3, 2)      
   self:place_portal_on_wall(8, 'stonehearth:portals:wooden_window_frame', 7, 2)      

   self:place_portal_on_wall(1, 'stonehearth:portals:wooden_window_frame', -2, 2)      
   self:place_portal_on_wall(1, 'stonehearth:portals:wooden_window_frame', -6, 2)      

   self:place_item_on_floor('stonehearth:furniture:not_much_of_a_bed', 3, 7, 0)
   self:place_item_on_floor('stonehearth:furniture:not_much_of_a_bed', 7, 7, 0)
   self:place_item_on_floor('stonehearth:furniture:not_much_of_a_bed', 11, 7, 0)
   self:place_item_on_floor('stonehearth:furniture:not_much_of_a_bed', 15, 7, 0)
end

function StonehearthTemplateBuilder:_create_template(o)
   -- fill in the ground.  this will erase the terrain cut of the previous
   local size = 512
   local region3 = Region3()
   local block_types = radiant.terrain.get_block_types()
   region3:add_cube(Cube3(Point3(-size,  -1, -size), Point3(size, 0, size), block_types.grass))
   radiant._root_entity:add_component('terrain')
                           :add_tile(region3)

   --
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
