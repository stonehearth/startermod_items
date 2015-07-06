local Point3 = _radiant.csg.Point3

local inventory_tests = {}

local function check_tracker_contents(autotest, name, contents)
   local tracking_data = stonehearth.inventory:get_inventory('player_1')
                                                   :get_item_tracker(name)
                                                      :get_tracking_data()

   -- got everything we expected...
   for k, data in pairs(tracking_data) do
      local expected = contents[k]
      if not expected then
         autotest:fail('unexpected key in %s inventory tracker: %s', name, k)         
      end
      if data.count ~= expected.count then
         autotest:fail('unexpected number of items in %s inventory tracker for %s', name, k)         
      end
   end

   for k, _ in pairs(contents) do
      if not tracking_data[k] then
         autotest:fail('%s tracker missing expected key %s', name, k)         
      end
   end
end

function inventory_tests.stockpile_contents(autotest)
   autotest.env:create_stockpile(10, 10, { size = { x = 2, y = 2 } })
   autotest.env:create_entity_cluster(10, 10, 2, 2, 'stonehearth:resources:wood:oak_log')

   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         ['stonehearth:resources:wood:oak_log'] = { count = 4 },
      })
   autotest:success()
end

function inventory_tests.crate_contents(autotest)
   local crate = autotest.env:create_entity(10, 10, 'stonehearth:containers:large_crate', { force_iconic = false })
   for i=1,4 do
      crate:get_component('stonehearth:storage')
               :add_item(radiant.entities.create_entity('stonehearth:resources:wood:oak_log'))
   end
   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         ['stonehearth:containers:large_crate'] = { count = 1 },
         ['stonehearth:resources:wood:oak_log'] = { count = 4 },
      })
   autotest:success()
end

function inventory_tests.destroy_entities(autotest)
   autotest.env:create_stockpile(10, 10, { size = { x = 2, y = 2 } })
   local logs = autotest.env:create_entity_cluster(10, 10, 2, 2, 'stonehearth:resources:wood:oak_log')

   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         ['stonehearth:resources:wood:oak_log'] = { count = 4 },
      })

   radiant.entities.destroy_entity(logs[1])
   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         ['stonehearth:resources:wood:oak_log'] = { count = 3 },
      })

   autotest:success()
end

function inventory_tests.placeable_no_root_entities(autotest)
   autotest.env:create_entity(10, 10, 'stonehearth:furniture:comfy_bed', { force_iconic = false })

   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         ['stonehearth:furniture:comfy_bed'] = { count = 1 },
      })
   check_tracker_contents(autotest, 'stonehearth:placeable_item_inventory_tracker', {
      })
   autotest:success()
end

function inventory_tests.placeable_entities(autotest)
   local bed = autotest.env:create_entity(10, 10, 'stonehearth:furniture:comfy_bed')
   local iconic_uri = bed:get_component('stonehearth:entity_forms')
                           :get_iconic_entity()
                              :get_uri()

   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         [iconic_uri] = { count = 1 },
      })
   check_tracker_contents(autotest, 'stonehearth:placeable_item_inventory_tracker', {
         [iconic_uri] = { count = 1 },
      })
   autotest:success()
end

--[[ busted!
function inventory_tests.placeable_removed_when_placed(autotest)
   local worker = autotest.env:create_person(8, 8, { job = 'worker' })
   local bed = autotest.env:create_entity(10, 10, 'stonehearth:furniture:comfy_bed')
   local iconic_bed = bed:get_component('stonehearth:entity_forms')
                                 :get_iconic_entity()
   local iconic_uri = iconic_bed:get_uri()

   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         [iconic_uri] = { count = 1 },
      })

   radiant.events.listen_once(worker, 'stonehearth:carry_block:carrying_changed', function()
         -- pickup should have taken it out of the placement tracker, since its
         -- parent became nil
         check_tracker_contents(autotest, 'stonehearth:placeable_item_inventory_tracker', {
            })
         autotest:success()
      end)

   autotest.ui:push_unitframe_command_button(iconic_bed, 'place_item')
   autotest.ui:click_terrain(0, 0)

   autotest:sleep(10000)
   autotest:fail()
end
]]

return inventory_tests
