local util = require 'tests.longrunning.build_longrunning_test_util'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local SimpleFloor = class()

function SimpleFloor:get_build_steps(autotest, session)
   return {
      function()
         local floor = stonehearth.build:add_floor(session,
                                                   'stonehearth:entities:wooden_floor',
                                                   Cube3(Point3(0, 1, 0), Point3(4, 2, 4)),
                                                   '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb')

         autotest.util:fail_if(util.get_area(floor) ~= 16, 'failed to create 4x4 floor blueprint')
      end
   }
end

return SimpleFloor
