local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local build_util = require 'stonehearth.lib.build_util'
local lrbt_util = require 'tests.longrunning.build.lrbt_util'

local BRICK_SLAB = 'stonehearth:build:voxel_brushes:voxel:stone:brick_tiled'
local STOREY_HEIGHT = stonehearth.constants.construction.STOREY_HEIGHT

local lrbt_cases = {}

function lrbt_cases.simple_floor(autotest, session)
   return {
      function()
         local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 9, 0), Point3(4, 10, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end
   }
end

function lrbt_cases.hybrid_floor(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(4, 11, 8)))
      end,
      function()
         floor = lrbt_util.create_stone_floor(session, Cube3(Point3(4, 10, 0), Point3(8, 11, 8)))
      end
   }
end

function lrbt_cases.simple_floor_erase(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(4, 11, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end,
      function()
         lrbt_util.erase_floor(session, Cube3(Point3(1, 10, 1), Point3(3, 11, 3)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 12, 'erase failed to remove center of floor blueprint')
      end,
   }
end

function lrbt_cases.simple_floor_merge(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(8, 11, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 32, 'failed to create 4x4 floor blueprint')
      end,
      function()
         lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(4, 11, 8)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 48, 'erase failed to remove center of floor blueprint')
      end,
   }
end

function lrbt_cases.three_way_floor_merge(autotest, session)
   local floor1, floor2
   -- make a U by connecting two parallel floors with a 3rd.  this test case was added because the merge
   -- will cause two buildings to become unlinked, which puts more stress on the ncz overlap computation path.
   -- it's a correctness and regression test.
   return {
      function()
         floor1 = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(2, 11, 6)))
         autotest.util:fail_if(lrbt_util.get_area(floor1) ~= 12, 'failed to create 2x6 floor blueprint')
      end,
      function()
         floor2 = lrbt_util.create_wooden_floor(session, Cube3(Point3(4, 10, 0), Point3(6, 11, 6)))
         autotest.util:fail_if(lrbt_util.get_area(floor2) ~= 12, 'failed to create 2x6 floor blueprint')
      end,
      function()
         lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(6, 11, 2)))
         autotest.util:fail_if(lrbt_util.get_area(floor1) ~= 28, 'failed to merge floors')
      end,
   }
end

function lrbt_cases.simple_wall(autotest, session)
   return {
      function()
         local wall = lrbt_util.create_wooden_wall(session, Point3(0, 10, 0), Point3(4, 10, 0))
         autotest.util:fail_if(lrbt_util.get_area(wall) ~= (3 * STOREY_HEIGHT), 'failed to create 4x4 floor blueprint')
      end,
   }
end


function lrbt_cases.wall_with_banner_by_item(autotest, session)
   local sign = autotest.env:create_entity(4, 8, 'stonehearth:decoration:wooden_sign_carpenter')
   local wall
   return {
      function()
         wall = lrbt_util.create_wooden_wall(session, Point3(4, 10, 0), Point3(0, 10, 0))
         autotest.util:fail_if(lrbt_util.get_area(wall) ~= (3 * STOREY_HEIGHT), 'failed to create 4x4 floor blueprint')
      end,
      function()
         local location = Point3(1, 4, 1)
         local normal = Point3(0, 0, 1)
         stonehearth.build:add_fixture(wall, sign, location, normal)
      end,
   }
end

function lrbt_cases.place_banner_by_type(autotest, session)
   local wall
   return {
      function()
         wall = lrbt_util.create_wooden_wall(session, Point3(4, 10, 0), Point3(0, 10, 0))
         autotest.util:fail_if(lrbt_util.get_area(wall) ~= (3 * STOREY_HEIGHT), 'failed to create 4x4 floor blueprint')
      end,
      function()
         local location = Point3(1, 4, 1)
         local normal = Point3(0, 0, 1)
         stonehearth.build:add_fixture(wall, 'stonehearth:decoration:wooden_sign_carpenter', location, normal)
      end,
   }
end


function lrbt_cases.place_bed_by_type(autotest, session)
   local floor
   return {
      function()
          floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(7, 11, 7)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 49, 'failed to create 4x4 floor blueprint')
      end,
      function()
         local location = Point3(3, 1, 3)
         local normal = Point3(0, 1, 0)
         stonehearth.build:add_fixture(floor, 'stonehearth:furniture:comfy_bed', location, normal, 0)
      end,
   }
end

function lrbt_cases.place_banner_by_type_flipped(autotest, session)
   local wall
   return {
      function()
         wall = lrbt_util.create_wooden_wall(session, Point3(4, 10, 0), Point3(0, 10, 0))
         autotest.util:fail_if(lrbt_util.get_area(wall) ~= (3 * STOREY_HEIGHT), 'failed to create 4x4 floor blueprint')
      end,
      function()
         local location = Point3(1, 4, -1)
         local normal = Point3(0, 0, -1)
         stonehearth.build:add_fixture(wall, 'stonehearth:decoration:wooden_sign_carpenter', location, normal)
      end,
   }
end

function lrbt_cases.grow_walls(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(4, 11, 4)))
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end,
      function()
         local building = build_util.get_building_for(floor)
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

function lrbt_cases.peaked_roof(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(5, 11, 5)))
      end,
      function()
         lrbt_util.grow_wooden_walls(session, floor)
      end,
      function()
         lrbt_util.grow_wooden_roof(session, build_util.get_building_for(floor))
      end,
   }
end

function lrbt_cases.patch_walls(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(8, 10, 0), Point3(16, 11, 4)))
      end,
      function()
         lrbt_util.create_wooden_floor(session, Cube3(Point3(8, 10, 0), Point3(12, 11, 6)))
      end,
      function()
         lrbt_util.grow_wooden_walls(session, floor)
      end,
      function()
         lrbt_util.grow_wooden_roof(session, build_util.get_building_for(floor))
      end,
   }
end

function lrbt_cases.floating_wall(autotest, session)
   return {
      function()
         lrbt_util.create_wooden_wall(session, Point3(5, 15, 5), Point3(12, 15, 5))
      end,
   }
end

function lrbt_cases.stacked_walls(autotest, session)
   local min, max = Point3(5, 10, 5), Point3(12, 10, 5)
   return {
      function()
         lrbt_util.create_wooden_wall(session, min, max)
      end,
      function()
         min.y = min.y + STOREY_HEIGHT
         max.y = max.y + STOREY_HEIGHT
         lrbt_util.create_wooden_wall(session, min, max)
      end,
   }
end

function lrbt_cases.stacked_walls_offset(autotest, session)
   local min, max = Point3(5, 10, 5), Point3(12, 10, 5)
   return {
      function()
         lrbt_util.create_wooden_wall(session, min, max)
      end,
      function()
         min.y = min.y + STOREY_HEIGHT
         max.y = max.y + STOREY_HEIGHT
         min.x = min.x + 1
         max.x = max.x - 1
         lrbt_util.create_wooden_wall(session, min, max)
      end,
   }
end

function lrbt_cases.two_storeys(autotest, session)
   local bounds = Cube3(Point3(0, 9, 0), Point3(4, 10, 4))
   local floor, second_floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, bounds)
         autotest.util:fail_if(lrbt_util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end,
      function()
         lrbt_util.grow_wooden_walls(session, floor)
      end,
      function()
         bounds:translate(Point3(0, STOREY_HEIGHT + 1, 0))
         bounds = bounds:inflated(Point3(1, 0, 1))
         second_floor = lrbt_util.create_wooden_floor(session, bounds)
      end,
      function()
         lrbt_util.grow_wooden_walls(session, second_floor)
      end,
      function()
         --lrbt_util.grow_wooden_roof(session, build_util.get_building_for(floor))
      end,
   }
end

function lrbt_cases.giant_slab(autotest, session)
   local slab
   local cube = Cube3(Point3(0, 9, 0), Point3(4, 15, 4))
   return {
      function()
         slab = stonehearth.build:add_floor(session, BRICK_SLAB, cube)
      end
   }
end


function lrbt_cases.obstructed_walls(autotest, session)
   return {
      function()
         lrbt_util.create_wooden_wall(session, Point3(0, 10, 0), Point3(4, 10, 0))
      end,
      function()
         lrbt_util.create_wooden_wall(session, Point3(0, 10, 1), Point3(4, 10, 1))
      end,
   }
end

function lrbt_cases.problem_shape_H(autotest, session)
   local floor
   return {
      function()
         floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 9, 0), Point3(8, 10, 4)))
         lrbt_util.create_wooden_floor(session, Cube3(Point3(3, 9, 0), Point3(5, 10, 10)))   -- crossbar
         lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 9, 6), Point3(8, 10, 10)))
      end,
      function()
         lrbt_util.grow_wooden_walls(session, floor)
      end,
      function()
         lrbt_util.grow_wooden_roof(session, build_util.get_building_for(floor))
      end,
   }
end

return lrbt_cases
