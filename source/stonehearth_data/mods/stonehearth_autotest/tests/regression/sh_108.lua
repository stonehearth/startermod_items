-- Build/Remove floor causes dudes to excavate ground anyway
-- https://bugs/browse/SH-108
--
-- Noticed this while Tony and I were building floor: if I go to build mode, put down
-- floor, and then remove the floor via the red button, the dudes still dig out the
-- place the floor would have been. Tony said he knows why this is happening, and
-- to assign the bug to Chris. 
--

local build_util = require 'stonehearth.lib.build_util'
local lrbt_util = require 'tests.longrunning.build.lrbt_util'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local sh_108 = {}

function sh_108.check_floor_merging(autotest)
   local buildings
   
   -- let's make some floor and finish it...
   buildings = lrbt_util.create_buildings(autotest, function(autotest, session)
         local wall = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 9, 0), Point3(5, 10, 5)))
      end)
   local _, finished_floor = next(buildings)
   stonehearth.build:instabuild(finished_floor)

   -- let's add another floor into it.  shouldn't merge!
   buildings = lrbt_util.create_buildings(autotest, function(autotest, session)
         local wall = lrbt_util.create_wooden_floor(session, Cube3(Point3(3, 9, 3), Point3(7, 10, 7)))
      end)
   local _, unfinished_floor = next(buildings)
   autotest.util:fail_if(unfinished_floor == finished_floor, 'merged unfinished floor into finished one!')

   -- just in case, remove the existing floor and wait around to see if we do any mining on the
   -- other one
   local workers = lrbt_util.create_workers(autotest, 2, 2)
   for _, worker in pairs(workers) do
      radiant.events.listen(worker, 'stonehearth:mined_location', function()
            autotest:fail('worker mined location for in-active sunk floor!')
         end)
   end
   stonehearth.build:set_teardown(finished_floor, true)

   autotest:sleep(3000) -- long enough that if they haven't mined, we're probably ok
   autotest:success()
end

return sh_108
