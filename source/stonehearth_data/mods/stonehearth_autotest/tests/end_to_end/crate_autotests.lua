local lrbt_util = require 'tests.longrunning.build.lrbt_util'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local crate_tests = {}

-- make sure we can simply restock the crate
function crate_tests.move_from_ground_into_crate(autotest)
   autotest.env:create_person(2, 2, { job = 'worker' })
   autotest.env:create_entity(4, 4, 'stonehearth:resources:wood:oak_log')
   autotest.env:create_entity(4, 5, 'stonehearth:resources:wood:oak_log')

   local crate = autotest.env:create_entity(0, 0, 'stonehearth:containers:small_crate', { force_iconic = false })
   local counter = 0
   radiant.events.listen(crate, 'stonehearth:storage:item_added', function()
         counter = counter + 1
         if counter == 2 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
   autotest:fail('worker did not put all the logs in the crate')
end

-- make sure we can place items from within the crate.
function crate_tests.place_item_from_crate(autotest)
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })
   local bed = autotest.env:create_entity(4, 4, 'stonehearth:furniture:comfy_bed')

   local crate = autotest.env:create_entity(0, 0, 'stonehearth:containers:small_crate', { force_iconic = false })

   radiant.events.listen_once(crate, 'stonehearth:storage:item_added', function()
         autotest.ui:click_dom_element('#build_menu')
         autotest.ui:click_dom_element('#place_item')
         autotest.ui:click_dom_element('div[uri="stonehearth:furniture:comfy_bed"]')
         autotest.ui:click_terrain(6, 6)
      end)

   local trace
   trace = radiant.entities.trace_grid_location(bed, 'sh crate placement autotest')
      :on_changed(function()
            local location = radiant.entities.get_world_grid_location(bed)
            if bed:get_component('mob'):get_parent() ~= nil then
               if location.x == 6 and location.z == 6 then
                  trace:destroy()
                  autotest:success()
                  return radiant.events.UNLISTEN
               end
            end
         end)


   autotest:sleep(15000)
   autotest:fail('worker did not put all the logs in the crate')
end

function crate_tests.dont_move_items_from_crate_to_crate(autotest)
   local session = autotest.env:get_player_session()
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })
   local crate = autotest.env:create_entity(0, 0, 'stonehearth:containers:small_crate', { force_iconic = false })
   local log = radiant.entities.create_entity('stonehearth:resources:wood:oak_log')

   radiant.entities.set_player_id(log, worker)
   crate:get_component('stonehearth:storage')
            :add_item(log)

   local trace = log:add_component('mob')
                        :trace_parent('making sure log doesn\'t move')
                           :on_changed(function()
                                 autotest:fail('worker picked up the log')
                              end)

   local other_crate = autotest.env:create_entity(4, 4, 'stonehearth:containers:small_crate', { force_iconic = false })

   autotest:sleep(2000)
   trace:destroy()
   autotest:success()
end

function crate_tests.move_items_from_crate_to_crate(autotest)
   local session = autotest.env:get_player_session()
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })
   local crate = autotest.env:create_entity(0, 0, 'stonehearth:containers:small_crate', { force_iconic = false })
   local log = radiant.entities.create_entity('stonehearth:resources:wood:oak_log')

   radiant.entities.set_player_id(log, worker)
   local sc = crate:get_component('stonehearth:storage')

   sc:add_item(log)
   sc:set_filter({ }) -- nothing

   local other_crate = autotest.env:create_entity(4, 4, 'stonehearth:containers:small_crate', { force_iconic = false })
   radiant.events.listen_once(other_crate, 'stonehearth:storage:item_added', function()
         autotest:success()
      end)

   autotest:sleep(5000)
   autotest:fail()
end

--[[

we're not sure if we actually want to do this.  disable autotest for now.

function crate_tests.move_items_from_crate_to_stockpile(autotest)
   local session = autotest.env:get_player_session()
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })
   local crate = autotest.env:create_entity(0, 0, 'stonehearth:containers:small_crate', { force_iconic = false })
   local log = radiant.entities.create_entity('stonehearth:resources:wood:oak_log')

   radiant.entities.set_player_id(log, worker)
   crate:get_component('stonehearth:storage')
            :add_item(log)

   local stockpile = autotest.env:create_stockpile(-2, -2)
   radiant.events.listen_once(stockpile, 'stonehearth:storage:item_added', function(e)
         if e.item == log then
            autotest:success()
         end
      end)

   autotest:sleep(1000000)
   autotest:fail()
end
]]

function crate_tests.build_from_crate(autotest)
   local session = autotest.env:get_player_session()
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })
   local crate = autotest.env:create_entity(0, 0, 'stonehearth:containers:small_crate', { force_iconic = false })
   local log = radiant.entities.create_entity('stonehearth:resources:wood:oak_log')
   
   radiant.entities.set_player_id(log, worker)
   crate:get_component('stonehearth:storage')
            :add_item(log)

   local buildings = lrbt_util.create_buildings(autotest, function(autotest, session)
         lrbt_util.create_wooden_floor(session, Cube3(Point3(4, 10, 4), Point3(6, 11, 6)))
      end)
      
   lrbt_util.succeed_when_buildings_finished(autotest, buildings)
   lrbt_util.mark_buildings_active(autotest, buildings)
   
   autotest:sleep(10000)
   autotest:fail('worker did not put all the logs in the crate')
end


function crate_tests.restock_items_in_backpack(autotest)
   local session = autotest.env:get_player_session()
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })

   local sc = worker:get_component('stonehearth:storage')
   for i=1,3 do
      local log = radiant.entities.create_entity('stonehearth:resources:wood:oak_log')
      sc:add_item(log)
   end

   autotest.env:create_stockpile(-4, -4)

   local listener
   listener = radiant.events.listen(radiant, 'stonehearth:gameloop', function()
         if sc:is_empty() then
            listener:destroy()
            autotest:success()
         end
      end)

   autotest:sleep(1000000)
   autotest:fail()
end

return crate_tests
