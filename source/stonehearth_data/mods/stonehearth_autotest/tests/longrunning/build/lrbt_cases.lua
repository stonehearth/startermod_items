local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local lrbt_cases = {}

local WOODEN_COLUMN = 'stonehearth:wooden_column'
local WOODEN_WALL = 'stonehearth:wooden_wall'
local WOODEN_FLOOR = 'stonehearth:entities:wooden_floor'
local WOODEN_FLOOR_PATTERN = '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb'
local STOREY_HEIGHT = stonehearth.constants.construction.STOREY_HEIGHT

local function get_building_for(structure)
   return stonehearth.build:get_building_for(structure)
end

local function get_area(structure)
   return structure:get_component('destination'):get_region():get():get_area()
end

local function count_structures(building)
   local counts = {}
   local structures = building:get_component('stonehearth:building')
                                 :get_all_structures()
   for type, entries in pairs(structures) do
      counts[type] = 0
      for _, entry in pairs(entries) do
         counts[type] = counts[type] + 1
      end
   end
   return counts
end

local function create_wooden_floor(session, cube)
   return stonehearth.build:add_floor(session, WOODEN_FLOOR, cube, WOODEN_FLOOR_PATTERN)
end

local function erase_floor(session, cube)
   stonehearth.build:erase_floor(session, cube)
end

local function create_wooden_wall(session, p0, p1)
   local t, n = 'x', 'z'
   if p0.x == p1.x then
      t, n = 'z', 'x'
   end
   local normal = Point3(0, 0, 0)
   if p0[t] < p1[t] then
      normal[n] = 1
   else
      p0, p1 = p1, p0
      normal[n] = -1
   end

   return stonehearth.build:add_wall(session, WOODEN_COLUMN, WOODEN_WALL, p0, p1, normal)
end

local function grow_wooden_walls(session, building)
   return stonehearth.build:grow_walls(building, WOODEN_COLUMN, WOODEN_WALL)
end

function lrbt_cases.simple_floor(autotest, session)
   return {
      function()
         local floor = create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 4)))
         autotest.util:fail_if(get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end
   }
end

function lrbt_cases.simple_floor_erase(autotest, session)
   local floor
   return {
      function()
         floor = create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 4)))
         autotest.util:fail_if(get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end,
      function()
         erase_floor(session, Cube3(Point3(1, 1, 1), Point3(3, 2, 3)))
         autotest.util:fail_if(get_area(floor) ~= 12, 'erase failed to remove center of floor blueprint')
      end,
   }
end

function lrbt_cases.simple_floor_merge(autotest, session)
   local floor
   return {
      function()
         floor = create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(8, 2, 4)))
         autotest.util:fail_if(get_area(floor) ~= 32, 'failed to create 4x4 floor blueprint')
      end,
      function()
         create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 8)))
         autotest.util:fail_if(get_area(floor) ~= 48, 'erase failed to remove center of floor blueprint')
      end,
   }
end

function lrbt_cases.simple_wall(autotest, session)
   return {
      function()
         local wall = create_wooden_wall(session, Point3(0, 1, 0), Point3(4, 1, 0))
         autotest.util:fail_if(get_area(wall) ~= (3 * STOREY_HEIGHT), 'failed to create 4x4 floor blueprint')
      end,
   }
end

function lrbt_cases.grow_walls(autotest, session)
   local floor
   return {
      function()
         floor = create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 4)))
         autotest.util:fail_if(get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end,
      function()
         local building = get_building_for(floor)
         grow_wooden_walls(session, building)

         local actual = count_structures(building)
         autotest.util:check_table(actual, {
               ['stonehearth:floor'] = 1,
               ['stonehearth:column'] = 4,
               ['stonehearth:wall'] = 4,
            })
      end,
   }
end


return lrbt_cases
