local build_util = require 'stonehearth.lib.build_util'
local lrbt_util = require 'tests.longrunning.build.lrbt_util'

local Point3 = _radiant.csg.Point3

local sh_80 = {}

function sh_80.verify_rotated_windows_in_template(autotest) 
   local centroid
   local rotation = 270
   local location = Point3(-4, 15, -4)
   
   centroid = lrbt_util.create_template(autotest, 'sh_80', function(autotest, session)
         local wall = lrbt_util.create_wooden_wall(session, Point3(-1, 10, 2), Point3(-7, 10, 2))
         stonehearth.build:add_fixture(wall, 'stonehearth:portals:wooden_window_frame', Point3(2, 2, 0))
      end)

   local buildings, scaffolding = lrbt_util.place_template(autotest, 'sh_80', location, centroid, rotation)

   -- look for windows...
   local traces = {}
   autotest.util:listen_while_running(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         if entity:get_uri() == 'stonehearth:portals:wooden_window_frame' then
            local mob = entity:get_component('mob')
            local trace = mob:trace_transform('autotest')
                                 :on_changed(function()
                                       local facing = mob:get_facing()
                                       if math.abs(facing - rotation) < 1 then
                                          autotest:success()
                                       else
                                          autotest:fail('facing %.2f does not equal expected value %.2f', facing, rotation)
                                       end
                                    end)
            table.insert(traces, trace)
         end
      end)

   -- fire it up!
   lrbt_util.fund_construction(autotest, buildings)
   --lrbt_util.mark_buildings_active(autotest, buildings)

   autotest:sleep(3000000)
   autotest:fail('failed to build before timeout')
end


return sh_80