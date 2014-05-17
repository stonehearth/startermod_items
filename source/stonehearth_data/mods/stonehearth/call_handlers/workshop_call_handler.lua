local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local WorkshopCallHandler = class()
local log = radiant.log.create_logger('call_handlers.worker')
local priorities = require('constants').priorities.worker_task
local IngredientList = require 'components.workshop.ingredient_list'

--- Client side object to add a new bench to the world.  this method is invoked
--  by POSTing to the route for this file in the manifest.
function WorkshopCallHandler:choose_workbench_location(session, response, workbench_entity)
   stonehearth.selection.select_place_location()
      :use_ghost_entity_cursor(workbench_entity)
      :done(function(selector, location, rotation)
            _radiant.call('stonehearth:create_ghost_workbench', workbench_entity, location, rotation)
                     :done(function (result)
                           response:resolve(result)
                        end)
                     :fail(function(result)
                           response:reject(result)
                        end)
                     :always(function ()
                           selector:destroy()
                        end)
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no region')
         end)
      :go()
end

--- Client side object to add the workbench's outbox to the world. 
function WorkshopCallHandler:choose_outbox_location(session, response, workbench_entity, crafter)
   stonehearth.selection.select_xz_region()
      :use_designation_marquee(Color4(0, 153, 255, 255))
      :set_cursor('stonehearth:cursors:create_stockpile')
      :done(function(selector, box)
            local size = Point2(box.max.x - box.min.x, box.max.z - box.min.z)
            _radiant.call('stonehearth:create_outbox', box.min, size, workbench_entity:get_id(), crafter:get_id())
                     :done(function(r)
                           response:resolve(r)
                        end)
                     :fail(function(o)
                           response:reject(o)
                        end)
                     :always(function()
                           selector:destroy()
                        end)
         end)
      :fail(function(selector)
            _radiant.call('stonehearth:destroy_ghost_workbench', workbench_entity:get_id())
            selector:destroy()
            response:reject('no region')
         end)
      :go()
end

--- Create a shadow of the workbench.
--  Workers are not actually asked to create the workbench until the outbox is placed too. 
function WorkshopCallHandler:create_ghost_workbench(session, response, workbench_entity_uri, pt, rotation)
   local location = Point3(pt.x, pt.y, pt.z)
   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   radiant.entities.set_faction(ghost_entity, session.faction)
   ghost_entity_component:set_full_sized_mod_uri(workbench_entity_uri)
   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)
   return {
      workbench_entity = ghost_entity
   }
end

--- Destroys a shadow of a workbench
function WorkshopCallHandler:destroy_ghost_workbench(session, response, ghost_entity_id)
   local ghost_entity = radiant.entities.get_entity(ghost_entity_id)
   radiant.entities.destroy_entity(ghost_entity)
end



--- Create the outbox the user specified and tell a worker to build the workbench
function WorkshopCallHandler:create_outbox(session, response, location, outbox_size, ghost_workshop_entity_id, crafter_id)
   local ghost_workshop = radiant.entities.get_entity(ghost_workshop_entity_id)
   local crafter = radiant.entities.get_entity(crafter_id)

   local crafter_component = crafter:get_component('stonehearth:crafter')
   if not crafter_component then
      return false
   end
   crafter_component:create_workshop(ghost_workshop, location, outbox_size)
   return true
end

return WorkshopCallHandler
