local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local lrbt = {}
local lrbt_cases = require 'tests.longrunning.build.lrbt_cases'

-- get the area of the blueprint for the given structure
--
local function get_blueprint_area(structure)
   return structure:get_component('destination'):get_region():get():get_area()
end

local function create_workers(autotest)
   autotest.env:create_person(-8,   8, { profession = 'worker' })
   autotest.env:create_person(-8,  10, { profession = 'worker' })
   autotest.env:create_person(-10,  8, { profession = 'worker' })
   autotest.env:create_person(-10, 10, { profession = 'worker' })
end

local function create_buildings(autotest, cb)
   local buildings = {}
   local building_listener = radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         if entity:is_valid() and entity:get_uri() == 'stonehearth:entities:building' then
            buildings[entity:get_id()] = entity
         end
      end)

   -- now run `cd` to create all the building structures.
   local session = autotest.env:get_player_session()
   local steps = cb(autotest, session)
   for i, step_cb in ipairs(steps) do
      stonehearth.build:do_command('step ' .. tostring(i), nil, function()
            step_cb()
         end)
   end

   building_listener:destroy()
   return buildings
end

local function mark_buildings_active(autotest, buildings)
   for _, building in pairs(buildings) do
      stonehearth.build:set_active(building, true)
   end
end

local function succeed_when_buildings_finished(autotest, buildings)
   for _, building in pairs(buildings) do
      radiant.events.listen(building, 'stonehearth:construction:finished_changed', function()
            if building:get_component('stonehearth:construction_progress'):get_finished() then
               buildings[building:get_id()] = nil
               if not next(buildings) then
                  autotest:success()
               end
               return radiant.events.UNLISTEN
            end
         end)
   end
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
   autotest.env:create_entity_cluster(8, 8, 8, 8, 'stonehearth:oak_log')

   local buildings = create_buildings(autotest, cb)

   mark_buildings_active(autotest, buildings)
   succeed_when_buildings_finished(autotest, buildings)
      
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
   lrbt['teardown_' .. name] = function(autotest)
      do_teardown(autotest, cb)
   end
end

return lrbt
