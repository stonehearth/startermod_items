local Point3 = _radiant.csg.Point3

local inventory_tests = {}

local function check_tracker_contents(autotest, name, contents)
   autotest:sleep(1)

   local tracking_data = stonehearth.inventory:get_inventory('player_1')
                                                   :get_item_tracker(name)
                                                      :get_tracking_data()
   for k, data in pairs(tracking_data) do
      local expected = contents[k]
      if not expected then
         autotest:fail('unexpected key in %s inventory tracker: %s', name, k)         
      end
      if data.count ~= expected.count then
         autotest:fail('unexpected number of items in %s inventory tracker for %s', name, k)         
      end
   end
end

function inventory_tests.basic_stockpile_contents(autotest)
   autotest.env:create_stockpile(10, 10, { size = { x = 2, y = 2 } })
   autotest.env:create_entity_cluster(10, 10, 2, 2, 'stonehearth:resources:wood:oak_log')

   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         ['stonehearth:resources:wood:oak_log'] = { count = 4 }
      })
   autotest:success()
end

function inventory_tests.basic_crate_contents(autotest)
   local crate = autotest.env:create_entity(10, 10, 'stonehearth:containers:large_crate', { force_iconic = false })
   for i=1,4 do
      crate:get_component('stonehearth:storage')
               :add_item(radiant.entities.create_entity('stonehearth:resources:wood:oak_log'))
   end
   check_tracker_contents(autotest, 'stonehearth:basic_inventory_tracker', {
         ['stonehearth:resources:wood:oak_log'] = { count = 4 }
      })
   autotest:success()
end

return inventory_tests
