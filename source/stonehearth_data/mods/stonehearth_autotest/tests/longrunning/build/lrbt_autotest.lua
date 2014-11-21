local build_util = require 'stonehearth.lib.build_util'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local lrbt = {}
local lrbt_util = require 'tests.longrunning.build.lrbt_util'
local lrbt_cases = require 'tests.longrunning.build.lrbt_cases'

local TEMPLATE_NAME = 'autotest_template'

-- get the area of the blueprint for the given structure
--
local function get_blueprint_area(structure)
   return structure:get_component('destination'):get_region():get():get_area()
end

-- run a build test
--
local function do_build(autotest, cb)
   local buildings, scaffolding = lrbt_util.create_buildings(autotest, cb)

   lrbt_util.fund_construction(autotest, buildings)
   lrbt_util.mark_buildings_active(autotest, buildings)
   lrbt_util.succeed_when_buildings_finished(autotest, buildings, scaffolding)
      
   autotest:sleep(60000)
   autotest:fail('failed to build structures')
end

-- run a template test
--
local function do_template(autotest, cb)
   local rotation = 90
   local location = Point3(-4, 15, -4)
   local centroid = lrbt_util.create_template(autotest, TEMPLATE_NAME, cb)

   local buildings, scaffolding = lrbt_util.place_template(autotest, TEMPLATE_NAME, location, centroid, rotation)

   lrbt_util.fund_construction(autotest, buildings)
   lrbt_util.mark_buildings_active(autotest, buildings)
   lrbt_util.succeed_when_buildings_finished(autotest, buildings, scaffolding)
      
   autotest:sleep(60000)
   autotest:fail('failed to build structures')
end

-- run a teardown test
--
local function do_teardown(autotest, cb)
   -- setup the environment for the build
   lrbt_util.create_workers(autotest, 18, 20)
   autotest.env:create_stockpile(-8, 8,  { size = { x = 6, y = 6 }})
   autotest.env:create_entity_cluster(8, 8, 8, 8, 'stonehearth:resources:wood:oak_log')

   local buildings = lrbt_util.create_buildings(autotest, cb)
   for _, building in pairs(buildings) do
      stonehearth.build:instabuild(building)
      stonehearth.build:set_teardown(building, true)
   end

   lrbt_util.mark_buildings_active(autotest, buildings)
   lrbt_util.succeed_when_buildings_destroyed(autotest, buildings)

   autotest:sleep(60000)
   autotest:fail('failed to teardown structures')
end

-- create all the long running build tests.  this is the cross product of the structurE_to_test
-- with all the different types of tests to run (build, teardown, etc.)
--
for name, cb in pairs(lrbt_cases) do   
   lrbt['build_' .. name] = function(autotest)
      do_build(autotest, cb)
   end
   lrbt['template_' .. name] = function(autotest)
      do_template(autotest, cb)
   end
   lrbt['teardown_' .. name] = function(autotest)
      do_teardown(autotest, cb)
   end
end


function lrbt.grow_walls_twice(autotest)
   local session = autotest.env:get_player_session()
   local expected = {
      ['stonehearth:floor'] = 1,
      ['stonehearth:column'] = 4,
      ['stonehearth:wall'] = 4,
   }
   local building
   

   stonehearth.build:do_command('create floor', nil, function()
         local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(0, 10, 0), Point3(4, 11, 1)))
         building = build_util.get_building_for(floor)
      end)

   for i=1,3 do
      stonehearth.build:do_command('grow_walls', nil, function()
            lrbt_util.grow_wooden_walls(session, building)
         end)

      local actual = lrbt_util.count_structures(building)
      autotest.util:fail_if_table_mismatch(expected, actual)
   end
   autotest:success()
end


function lrbt.grow_walls_perf_test(autotest)
   local session = autotest.env:get_player_session()
   local building

   autotest.ui:click_dom_element('#startMenu #build_menu')

   local w=25
   for i=-w,w, 5 do
      stonehearth.build:do_command('create floor', nil, function()
            local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(i, 10, -w), Point3(i+2, 11, w+2)))
         end)
      stonehearth.build:do_command('create floor', nil, function()
            local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(-w, 10, i), Point3(w, 11, i+2)))
            building = build_util.get_building_for(floor)
         end)
   end
   autotest:sleep(1)

   local now = radiant.get_realtime()
   autotest:profile(function()
         stonehearth.build:do_command('grow wall', nil, function()
               lrbt_util.grow_wooden_walls(session, building)
            end)
      end)
   local elapsed = radiant.get_realtime() - now

   autotest:log('growing walls finished! (elapsed: %.2f seconds)', elapsed)
   autotest:sleep(2000)
   autotest:success()
end

function lrbt.shanty_town(autotest)
   local session = autotest.env:get_player_session()
   local building

   lrbt_util.create_endless_entity(autotest, -31, 0, 1, 8, 'stonehearth:resources:wood:oak_log')
   lrbt_util.create_endless_entity(autotest, -30, 0, 1, 8, 'stonehearth:food:berries:berry_basket')
   lrbt_util.create_workers(autotest)

   local function create_house(x, y)
      local house
      stonehearth.build:do_command('house', nil, function()
            local w, h = 3, 3
            local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(x, 10, y), Point3(x + w, 11, y + h)))
            house = build_util.get_building_for(floor)
            lrbt_util.grow_wooden_walls(session, house)
            lrbt_util.grow_wooden_roof(session, house)
         end)
      return house
   end

   for y = -24, 24, 9 do
      for x = -24, 24, 9 do
         local house = create_house(x, y)
         stonehearth.build:set_active(house, true)
      end
   end

   while true do
      autotest:sleep(1000)
   end
end

function lrbt.expensive_building(autotest)
   local session = autotest.env:get_player_session()
   local building

   lrbt_util.create_endless_entity(autotest, 8, 16, 2, 4, 'stonehearth:resources:wood:oak_log')
   lrbt_util.create_endless_entity(autotest, -8, 16, 2, 4, 'stonehearth:food:berries:berry_basket')
   lrbt_util.create_workers(autotest)

   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest:sleep(1000)

   stonehearth.build:do_command('create floor', nil, function()
         local floor = lrbt_util.create_wooden_floor(session, Cube3(Point3(-20, 10, -20), Point3(18, 11, 10)))
         building = build_util.get_building_for(floor)
      end)

   local w=18
   for i=-w,w-1, 6 do
      stonehearth.build:do_command('create floor', nil, function()
            lrbt_util.create_wooden_floor(session, Cube3(Point3(i, 1, -22), Point3(i+3, 2, -20)))
            lrbt_util.create_wooden_floor(session, Cube3(Point3(i, 1, 10), Point3(i+3, 2, 12)))
         end)
   end
   stonehearth.build:do_command('grow wall', nil, function()
         lrbt_util.grow_wooden_walls(session, building)
      end)
   stonehearth.build:do_command('grow roof', nil, function()
         lrbt_util.grow_wooden_roof(session, building)
      end)

   stonehearth.build:set_active(building, true)

   autotest:sleep(200000000)
   autotest:success()
end

lrbt.shanty_town = nil
lrbt.expensive_building = nil
lrbt.grow_walls_perf_test = nil

return lrbt
