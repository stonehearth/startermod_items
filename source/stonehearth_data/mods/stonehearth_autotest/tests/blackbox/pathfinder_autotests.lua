local lrbt_util = require 'tests.longrunning.build.lrbt_util'
local build_util = require 'stonehearth.lib.build_util'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3


local pathfinder_tests = {}

function pathfinder_tests.check_movement_guard(autotest)
   local buildings
   
   autotest.ui:set_building_vision_mode('rpg')

   -- create a building with a door in it
   buildings = lrbt_util.create_buildings(autotest, function(autotest, session)
         local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(-4, 9, -4), Point3(4, 10, 4)))
         local building = build_util.get_building_for(floor)
         lrbt_util.grow_wooden_walls(session, building)

         local structures = building:get_component('stonehearth:building')
                              :get_all_structures()
         local id, entry = next(structures['stonehearth:wall'])
         id, entry = next(structures['stonehearth:wall'], id)
         id, entry = next(structures['stonehearth:wall'], id)

         local wall = entry.entity
         local normal = wall:get_component('stonehearth:construction_data')
                                 :get_normal()

         local bounds = wall:get_component('destination')
                                 :get_region()
                                    :get()
                                       :get_bounds()

         local location = Point3(0, 0, 0)
         if normal.x == 0 then
            location.x = (bounds.min.x + bounds.max.x) / 2
         else
            location.z = (bounds.min.z + bounds.max.z) / 2
         end
         stonehearth.build:add_fixture(wall, 'stonehearth:portals:wooden_door', location, normal)
      end)

   for _, building in pairs(buildings) do
      stonehearth.build:instabuild(building)
   end

   local gold = autotest.env:create_entity(0, 0, 'stonehearth:loot:gold')

   -- make sure a sheep can't!
   local sheep = autotest.env:create_entity(0, 18, 'stonehearth:sheep')
   local pathfinder = _radiant.sim.create_astar_path_finder(sheep, 'check_movement_guard test')
                                    :add_destination(gold)
                                    :set_solved_cb(function(path)
                                          autotest:fail('sheep made it through the door')
                                          return true
                                       end)
                                    :set_search_exhausted_cb(function()
                                          autotest:resume()
                                       end)
                                    :start()                                   
   autotest:suspend()
   pathfinder:destroy()
   radiant.entities.destroy_entity(sheep)

   -- make sure a person can get in...
   local person = autotest.env:create_person(0, 18, { job = 'shepherd' })
   local pathfinder = _radiant.sim.create_astar_path_finder(person, 'check_movement_guard test')
                                    :add_destination(gold)
                                    :set_solved_cb(function(path)
                                          autotest:resume()
                                          return true
                                       end)
                                    :set_search_exhausted_cb(function()
                                          autotest:fail('citizen could not find path through door')
                                       end)
                                    :start()

   autotest:suspend()
   pathfinder:destroy()
   autotest:success()
end

return pathfinder_tests
