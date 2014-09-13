local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local lrbt_cases = {}
local lrbt_util = require 'tests.longrunning.build.lrbt_util'

local WOODEN_COLUMN = 'stonehearth:wooden_column'
local WOODEN_WALL = 'stonehearth:wooden_wall'
local WOODEN_FLOOR = 'stonehearth:entities:wooden_floor'
local WOODEN_FLOOR_PATTERN = '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb'
local STOREY_HEIGHT = stonehearth.constants.construction.STOREY_HEIGHT


function lrbt_cases.simple_floor(autotest, session)
   return {
      function()
         local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end
   }
end

function lrbt_cases.simple_floor_erase(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end,
      function()
         lrbt_util.erase_floor(session, Cube3(Point3(1, 1, 1), Point3(3, 2, 3)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 12, 'erase failed to remove center of floor blueprint')
      end,
   }
end

function lrbt_cases.simple_floor_merge(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(8, 2, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 32, 'failed to create 4x4 floor blueprint')
      end,
      function()
         lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 8)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 48, 'erase failed to remove center of floor blueprint')
      end,
   }
end

function lrbt_cases.simple_wall(autotest, session)
   return {
      function()
         local wall = lrbt_util.create_wooden_wall(session, Point3(0, 1, 0), Point3(4, 1, 0))
         autotest.util:fail_if(lrbt_util.get_area(wall) ~= (3 * STOREY_HEIGHT), 'failed to create 4x4 floor blueprint')
      end,
   }
end

function lrbt_cases.grow_walls(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 1, 0), Point3(4, 2, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end,
      function()
         local building = stonehearth.build:get_building_for(floor)
         lrbt_util.grow_wooden_walls(session, building)

         local expected = {
            ['stonehearth:floor'] = 1,
            ['stonehearth:column'] = 4,
            ['stonehearth:wall'] = 4,
         }
         local actual = lrbt_util.count_structures(building)
         autotest.util:fail_if_table_mismatch(expected, actual)
      end,
   }
end


return lrbt_cases
