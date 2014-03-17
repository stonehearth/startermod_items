local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Color3 = _radiant.csg.Color3
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local priorities = require('constants').priorities.worker_task
local farming_service = stonehearth.farming

local ResourceCallHandler = class()


function ResourceCallHandler:box_harvest_resources(session, response)
   self._region = _radiant.client.alloc_region2()
   
   local cursor_entity = radiant.entities.create_entity()
   local mob = cursor_entity:add_component('mob')
   local cursor_render_entity = _radiant.client.create_render_entity(1, cursor_entity)
   
   local parent_node = cursor_render_entity:get_node()
   local node

   local cursor = _radiant.client.set_cursor('stonehearth:cursors:harvest')

   local cleanup = function()
      if node then
         h3dRemoveNode(node)
      end
      cursor:destroy()
      _radiant.client.destroy_authoring_entity(cursor_entity:get_id())
   end

   _radiant.client.select_xz_region(1)
      :progress(function (box)
            self._region:modify(function(cursor)
               cursor:clear()
               cursor:add_cube(Rect2(Point2(0, 0), 
                                     Point2(box.max.x - box.min.x + 1, box.max.z - box.min.z + 1)))
            end)
            mob:set_location_grid_aligned(box.min)
            if node then
               h3dRemoveNode(node)
            end
            node = _radiant.client.create_designation_node(parent_node, self._region:get(), Color3(0, 255, 0), Color3(0, 255, 0));
         end)
      :done(function (box)
            _radiant.call('stonehearth:server_box_harvest_resources', box)
         end)
      :always(function()
            cleanup()
         end)   
      :fail(function()
            response:resolve(false)
         end)   
end


function ResourceCallHandler:server_box_harvest_resources(session, response, box)
   local cube = Cube3(Point3(box.min.x, box.min.y, box.min.z),
                      Point3(box.max.x, box.max.y, box.max.z))

   for i, entity in radiant.terrain.get_entities_in_box(cube) do
      if entity:get_component('stonehearth:renewable_resource_node') then
         self:harvest_plant(session, response, entity)
      elseif entity:get_component('stonehearth:resource_node') then
         self:harvest_tree(session, response, entity)
      end
   end
end

--- Remote entry point for requesting that a tree get harvested
-- @param tree The entity which you would like chopped down
-- @return true on success, false on failure
function ResourceCallHandler:harvest_tree(session, response, tree)
   local town = stonehearth.town:get_town(session.player_id)
   return town:harvest_resource_node(tree)
end

function ResourceCallHandler:harvest_plant(session, response, plant)
   local town = stonehearth.town:get_town(session.player_id)
   return town:harvest_renewable_resource_node(plant)
end

function ResourceCallHandler:shear_sheep(session, response, sheep)
   asset(false, 'shear_sheep was broken, so i deleted it =)  -tony')
end


return ResourceCallHandler
