-- Tiny ladder gets built underground in front of doors
-- https://bugs/browse/SH-81
--
-- At some point while building a wall with a door in it, a 1 voxel high
-- ladder actually gets placed in front of the door.
--
-- This screenshot is the mining visualization 1 layer below a house under
-- construction. Note the ladder.
--

local build_util = require 'stonehearth.lib.build_util'
local lrbt_util = require 'tests.longrunning.build.lrbt_util'

local Point3 = _radiant.csg.Point3

local sh_81 = {}

function sh_81.verify_no_ladder_inside_door(autotest)

   local buildings, scaffolding = lrbt_util.create_buildings(autotest, function(autotest, session)
         local wall = lrbt_util.create_wooden_wall(session, Point3(-7, 10, 2), Point3(-1, 10, 2))
         stonehearth.build:add_fixture(wall, 'stonehearth:portals:wooden_door_2', Point3(-2, 0, 0))
      end)

   -- if we create a ladder while making this building, we have failed.
   autotest.util:fail_if_created('stonehearth:build:prototypes:ladder')

   -- fire it up!
   lrbt_util.fund_construction(autotest, buildings)
   lrbt_util.mark_buildings_active(autotest, buildings)
   lrbt_util.succeed_when_buildings_finished(autotest, buildings, scaffolding)

   autotest:sleep(30000)
end


return sh_81