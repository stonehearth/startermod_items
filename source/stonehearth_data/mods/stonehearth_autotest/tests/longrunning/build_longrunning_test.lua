
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local structures_to_test = {}
local build_longrunning_tests = {}

-- get the area of the blueprint for the given structure
--
local function get_blueprint_area(structure)
   return structure:get_component('destination'):get_region():get():get_area() 
end

-- run a build test
--
local function do_build(autotest, cb)
   -- setup the environment for the build
   autotest.env:create_person(-8,   8, { profession = 'worker' })   
   autotest.env:create_person(-8,  10, { profession = 'worker' })   
   autotest.env:create_person(-10,  8, { profession = 'worker' })   
   autotest.env:create_person(-10, 10, { profession = 'worker' })   
   autotest.env:create_entity_cluster(8, 8, 8, 8, 'stonehearth:oak_log')

   -- now run `cd` to create all the building structures.
   local session = autotest.env:get_player_session()
   local structure
   stonehearth.build:do_command('', nil, function()
         structure = cb(session)
      end)

   -- fetch the building and start it up!
   local building = stonehearth.build:get_building_for(structure)
   stonehearth.build:set_active(building, true)

   -- signal success when the building is completed.
   radiant.events.listen(building, 'stonehearth:construction:finished_changed', function()
         if building:get_component('stonehearth:construction_progress'):get_finished() then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)
      
   autotest:sleep(60000)
   autotest:fail('failed to build structure')
end

-- run a teardown test
--
local function do_teardown(autotest, cb)
   autotest.env:create_person(-8,   8, { profession = 'worker' })   
   autotest.env:create_person(-8,  10, { profession = 'worker' })   
   autotest.env:create_person(-10,  8, { profession = 'worker' })   
   autotest.env:create_person(-10, 10, { profession = 'worker' })   
   autotest.env:create_stockpile(-8, 8,  { size = { x = 6, y = 6 }})

   -- now run `cd` to create all the building structures.
   local session = autotest.env:get_player_session()
   local structure
   stonehearth.build:do_command('', nil, function()
         structure = cb(session)
      end)

   -- fetch the building and start it up!
   local building = stonehearth.build:get_building_for(structure)
   stonehearth.build:instabuild(building)
   stonehearth.build:set_teardown(building, true)
   stonehearth.build:set_active(building, true)

   -- signal success when the building is completed.
   radiant.events.listen(building, 'stonehearth:construction:finished_changed', function()
         if building:get_component('stonehearth:construction_progress'):get_finished() then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)
      
   autotest:sleep(60000)
   autotest:fail('failed to teardown structure')
end

-- a simple floor structure
function structures_to_test.simple_floor(session)
   return stonehearth.build:add_floor(session,
                                      'stonehearth:entities:wooden_floor',
                                      Cube3(Point3(0, 1, 0), Point3(4, 2, 4)),
                                      '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb')
end

-- create all the long running build tests.  this is the cross product of the structurE_to_test
-- with all the different types of tests to run (build, teardown, etc.)
--
for fname, fn in pairs(structures_to_test) do   
   build_longrunning_tests['build_' .. fname] = function(autotest)
      do_build(autotest, fn)
   end
   build_longrunning_tests['teardown_' .. fname] = function(autotest)
      do_teardown(autotest, fn)
   end
end

return build_longrunning_tests