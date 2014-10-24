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

local function create_workers(autotest)
   autotest.env:create_person(-8,   8, { job = 'worker' })
   autotest.env:create_person(-8,  10, { job = 'worker' })
   autotest.env:create_person(-10,  8, { job = 'worker' })
   autotest.env:create_person(-10, 10, { job = 'worker' })
end

local function create_log_pile(autotest)
   for y=8,12 do
      for x=8,12 do
         lrbt_util.create_endless_entity(autotest, x, y, 'stonehearth:oak_log')
      end
   end
end

local function track_buildings_and_scaffolding_creation(cb)
   local scaffolding, buildings = {}, {}

   local building_listener = radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         if entity:is_valid() and entity:get_uri() == 'stonehearth:entities:building' then
            buildings[entity:get_id()] = entity
         end
         if entity:is_valid() and entity:get_uri() == 'stonehearth:scaffolding' then
            scaffolding[entity:get_id()] = entity
         end
      end)

   cb()

   building_listener:destroy()
   
   -- the act of running through all the steps may have deleted some of these objects.
   -- remove them from the arrays
   stonehearth.build:clear_undo_stack()
   
   local function prune_dead_entities(t)
      for id, entity in pairs(t) do
         if not entity:is_valid() then
            t[id] = nil
         end
      end
   end
   
   prune_dead_entities(buildings)
   prune_dead_entities(scaffolding)

   return buildings, scaffolding
end

local function create_buildings(autotest, cb)
   local buildings, scaffolding = track_buildings_and_scaffolding_creation(function()
         -- now run `cd` to create all the building structures.
         local session = autotest.env:get_player_session()
         local steps = cb(autotest, session)
         for i, step_cb in ipairs(steps) do
            stonehearth.build:do_command('step ' .. tostring(i), nil, function()
                  step_cb()
               end)
         end
      end)

   return buildings, scaffolding
end

local function create_template(autotest, cb)
   local buildings = create_buildings(autotest, cb)

   -- template test only works with the first building
   local _, building = next(buildings)
   local centroid = build_util.get_building_centroid(building)
   build_util.save_template(building, { name = TEMPLATE_NAME }, true)

   -- nuke the buildings
   for _, b in pairs(buildings) do
      radiant.entities.destroy_entity(b)
   end
   
   return centroid
end

local function place_template(autotest, centroid)
   -- drop the template
   local buildings, scaffolding = track_buildings_and_scaffolding_creation(function()
         local session = autotest.env:get_player_session()
         stonehearth.build:do_command('place template', nil, function()
               stonehearth.build:build_template(session, TEMPLATE_NAME, Point3(-4, 15, -4), centroid, 90)
            end)
      end)

   return buildings, scaffolding
end

local function mark_buildings_active(autotest, buildings)
   for _, building in pairs(buildings) do
      stonehearth.build:set_active(building, true)
   end
end

local function succeed_when_buildings_finished(autotest, buildings, scaffoldings)
   local succeeded = false
   local function succeed_if_finished()
      if not succeeded then
         for _, building in pairs(buildings) do
            if not building:get_component('stonehearth:construction_progress'):get_finished() then
               return
            end
         end
         for _, scaffolding in pairs(scaffoldings) do
            if not scaffolding:get_component('destination'):get_region():get():empty() then
               return
            end
         end
         succeeded = true
         autotest:success()
      end
   end
   
   local function install_listeners(t)
      for _, entity in pairs(t) do
         radiant.events.listen(entity, 'stonehearth:construction:finished_changed', function()
               succeed_if_finished();
            end)
      end
   end
   
   install_listeners(buildings)
   install_listeners(scaffoldings)
end

local function succeed_when_buildings_destroyed(autotest, buildings)
   for _, building in pairs(buildings) do
      radiant.events.listen_once(building, 'radiant:entity:pre_destroy', function()
            buildings[building:get_id()] = nil
            if not next(buildings) then
               autotest:success()
            end
         end)
   end
end

-- run a build test
--
local function do_build(autotest, cb)
   -- setup the environment for the build
   create_workers(autotest)
   create_log_pile(autotest)

   local buildings, scaffolding = create_buildings(autotest, cb)

   mark_buildings_active(autotest, buildings)
   succeed_when_buildings_finished(autotest, buildings, scaffolding)
      
   autotest:sleep(60000)
   autotest:fail('failed to build structures')
end

-- run a template test
--
local function do_template(autotest, cb)
   -- setup the environment for the build
   create_workers(autotest)
   create_log_pile(autotest)

   local centroid = create_template(autotest, cb)
   local buildings, scaffolding = place_template(autotest, centroid)

   mark_buildings_active(autotest, buildings)
   succeed_when_buildings_finished(autotest, buildings, scaffolding)
      
   autotest:sleep(60000)
   autotest:fail('failed to build structures')
end

-- run a teardown test
--
local function do_teardown(autotest, cb)
   -- setup the environment for the build
   create_workers(autotest)
   autotest.env:create_stockpile(-8, 8,  { size = { x = 6, y = 6 }})
   autotest.env:create_entity_cluster(8, 8, 8, 8, 'stonehearth:oak_log')

   local buildings = create_buildings(autotest, cb)
   for _, building in pairs(buildings) do
      stonehearth.build:instabuild(building)
      stonehearth.build:set_teardown(building, true)
   end

   mark_buildings_active(autotest, buildings)
   succeed_when_buildings_destroyed(autotest, buildings)

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

function lrbt.expensive_building(autotest)
   local session = autotest.env:get_player_session()
   local building

   for i = 0,8 do
      lrbt_util.create_endless_entity(autotest, 8 + i % 4, 16 + i / 4, 'stonehearth:oak_log')
      lrbt_util.create_endless_entity(autotest, -8 + i % 4, 16 + i / 4, 'stonehearth:berry_basket')
   end
   create_workers(autotest)

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

lrbt.expensive_building = nil
lrbt.grow_walls_perf_test = nil

return lrbt
