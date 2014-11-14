local build_util = require 'stonehearth.lib.build_util'
local entity_forms = require 'stonehearth.lib.entity_forms.entity_forms_lib'
local PlaceItemCallHandler = require 'stonehearth.call_handlers.place_item_call_handler'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local placement_autotests = {}

-- Test that 2 workers can compete to move the bed to a target location
-- Starting from a proxy, place it and then move it several times
--
function placement_autotests.two_place_multiple_times(autotest)
   autotest.env:create_person(-8, 8, { job = 'worker' })
   autotest.env:create_person(20, -20, { job = 'worker' })

   local big_bed = autotest.env:create_entity(0, 0, 'stonehearth:furniture:comfy_bed')
   local bed_proxy = big_bed:get_component('stonehearth:entity_forms')
                                 :get_iconic_entity()

   --If the big bed moves to the target location, we win!
   local trace
   trace = radiant.entities.trace_grid_location(big_bed, 'sh placement autotest')
      :on_changed(function()
            local location = radiant.entities.get_world_grid_location(big_bed)
            -- Check to see if the mob has a parent, meaning we're in the world, and not being
            -- removed.  The trace fires when being removed, AND the location is set to 0,0,0,
            -- which we want to ignore.
            if big_bed:get_component('mob'):get_parent() ~= nil then
               if location.x == 8 and location.z == 8 then
                  trace:destroy()
                  autotest:success()
                  return radiant.events.UNLISTEN
               else
                  autotest.ui:push_unitframe_command_button(big_bed, 'move_item')
                  autotest.ui:click_terrain(location.x + 2, location.z + 2)
               end
            end
         end)
   
   autotest.ui:push_unitframe_command_button(bed_proxy, 'place_item')
   autotest.ui:click_terrain(2, 2)

   autotest:sleep(60000)
   autotest:fail('worker failed to move the bed')
end

-- make sure we can place items on walls
--
function placement_autotests.place_on_wall(autotest)
   autotest.env:create_person(-8, 8, { job = 'worker' })
   autotest.env:create_entity(5, 8, 'stonehearth:oak_log')

   local stockpile = autotest.env:create_stockpile(4, 8)
   local sign = autotest.env:create_entity(4, 8, 'stonehearth:decoration:wooden_sign_carpenter')
   
   local session = autotest.env:get_player_session()
   radiant.entities.set_player_id(sign, session.player_id)

   local wall, normal, building

   -- create a wall super fast.
   stonehearth.build:do_command('place_on_wall', nil, function()
         normal = Point3(0, 0, 1)
         wall = stonehearth.build:add_wall(session,
                                           'stonehearth:wooden_column',
                                           'stonehearth:wooden_wall',
                                           Point3(-2, 10, 2),
                                           Point3( 2, 10, 2),
                                           normal)
         building = build_util.get_building_for(wall)
      end)
   stonehearth.build:instabuild(building)
   
   -- place the signe on the wall
   local placement_location = Point3(1, 15, 3)
   local location = placement_location - radiant.entities.get_world_grid_location(wall)
   stonehearth.build:add_fixture(wall, sign, location, normal)

   local trace = radiant.entities.trace_grid_location(sign, 'find path to entity')
      :on_changed(function()         
            if radiant.entities.get_world_grid_location(sign) == placement_location then
               -- by now the sign is on the wall.   make sure the ladder gets torn down
               radiant.events.listen(radiant, 'radiant:entity:pre_destroy', function(e)
                     if e.entity:get_component('stonehearth:ladder') then
                        autotest:success()
                     end
                     return radiant.events.UNLISTEN
                  end)
            end
         end)
      
   autotest:sleep(10000)
   autotest:fail('worker failed to place sign and remove ladder')
end

local function place_on_cliff_helper(autotest, item_or_type)
   autotest.env:create_person(-8, 8, { job = 'worker' })   
   autotest.env:create_entity(5, 8, 'stonehearth:oak_log')

   local sign
   local stockpile = autotest.env:create_stockpile(4, 8)
   if item_or_type == 'item' then
      sign = autotest.env:create_entity(4, 8, 'stonehearth:decoration:wooden_sign_carpenter')
   end
   
   local session = autotest.env:get_player_session()
   radiant.entities.set_player_id(sign, session.player_id)

   -- make a cliff
   local block_types = radiant.terrain.get_block_types()
   local cliff = Cube3(Point3(10, 11, 10), Point3(16, 19, 16), block_types.bedrock)

   radiant._root_entity:get_component('terrain')
                           :add_cube(cliff)
  
   autotest:sleep(100) -- let inventory triggers fire...

   -- place the sign on the wall
   local placement_location = Point3(12, 13, 16)
   local normal = Point3.unit_z;
   local response = { resolve = function() end, reject = function() end }

   if item_or_type == 'item' then
      PlaceItemCallHandler():place_item_in_world(session, nil, sign, placement_location, 180, normal)
   else 
      local _, iconic_uri, _ = entity_forms.get_uris('stonehearth:decoration:wooden_sign_carpenter')
      PlaceItemCallHandler():place_item_type_in_world(session, response, iconic_uri, placement_location, 180, normal)
   end

   local trace = radiant.entities.trace_grid_location(sign, 'find path to entity')
      :on_changed(function()         
            if radiant.entities.get_world_grid_location(sign) == placement_location then
               -- by now the sign is on the wall.   make sure the ladder gets torn down
               radiant.events.listen(radiant, 'radiant:entity:pre_destroy', function(e)
                     if e.entity:get_component('stonehearth:ladder') then
                        radiant._root_entity:get_component('terrain')
                                                :subtract_cube(cliff)
                        autotest:success()
                     end
                     return radiant.events.UNLISTEN
                  end)
            end
         end)
      
   autotest:sleep(10000000)
   autotest:fail('worker failed to place sign and remove ladder')   
end

function placement_autotests.place_sign_on_cliff_by_item(autotest)
   place_on_cliff_helper(autotest, 'item')
end

function placement_autotests.place_sign_on_cliff_by_type(autotest)
   place_on_cliff_helper(autotest, 'item')
end

function placement_autotests.undeploy_item(autotest)
   autotest.env:create_person(-8, 8, { job = 'worker' })   
   local big_bed = autotest.env:create_entity(8, 8, 'stonehearth:furniture:comfy_bed')
   local efc = big_bed:get_component('stonehearth:entity_forms')
   local iconic_bed = efc:get_iconic_entity()

   local stockpile = autotest.env:create_stockpile(0, 0)

   --If the big bed moves to the target location, we win!
   local trace
   trace = radiant.entities.trace_grid_location(iconic_bed, 'sh undeploy autotest')
      :on_changed(function()
            local location = radiant.entities.get_world_grid_location(iconic_bed)
            -- Check to see if the mob has a parent, meaning we're in the world, and we're
            -- in iconic form, meaning we've been dropped in a stockpile
            if iconic_bed:get_component('mob'):get_parent() ~= nil then
               if location.x < 2 and location.z < 2 then
                  trace:destroy()
                  autotest:success()
                  return radiant.events.UNLISTEN
               end
            end
         end)

   autotest.ui:push_unitframe_command_button(big_bed, 'undeploy_item')

   autotest:sleep(20 * 1000)
   autotest:fail('worker failed to pickup bed and move it to a stockpile')
end

return placement_autotests