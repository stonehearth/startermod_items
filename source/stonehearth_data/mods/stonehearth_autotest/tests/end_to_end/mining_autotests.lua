local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local mining_tests = {}

function mining_tests.down_and_out_tunnel(autotest)
   local session = autotest.env:get_player_session()

   autotest.env:create_person(15, 0, { job = 'worker' })
   autotest.env:create_person(15, 5, { job = 'worker' })
   autotest.env:create_person(15, 10, { job = 'worker' })
   autotest.env:create_person(15, 15, { job = 'worker' })

   local point = Point3(20, 9, 4)
   local region = Region3(Cube3(point, point + Point3(1, 1, 1)))

   local mining_zone = stonehearth.mining:dig_down(session.player_id, region)
   if not mining_zone then
      autotest:fail('Mining test failed. Is insta_mine turned on?')
   end

   for i = 1, 2 do
      region:translate(Point3(4, 0, 0))
      stonehearth.mining:dig_out(session.player_id, region)
   end

   local location = radiant.entities.get_world_grid_location(mining_zone)
   local mining_zone_component = mining_zone:add_component('stonehearth:mining_zone')
   local mining_bounds = mining_zone_component:get_region():get():translated(location)
   local saved_terrain = radiant.terrain.intersect_region(mining_bounds)

   autotest.ui:click_dom_element('#xrayButton')

   radiant.events.listen(mining_zone, 'radiant:entity:pre_destroy', function()
         autotest.ui:click_dom_element('#xrayButton')
         radiant.terrain.add_region(saved_terrain)
         autotest:success()
      end)

   autotest:sleep(80000)
   autotest:fail('down_and_out_tunnel failed to complete on time')
end

return mining_tests
