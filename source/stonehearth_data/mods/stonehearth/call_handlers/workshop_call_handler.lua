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
function WorkshopCallHandler:choose_workbench_location(session, response, workbench_entity, crafter)
   stonehearth.selection:select_location()
      :use_ghost_entity_cursor(workbench_entity)
      :done(function(selector, location, rotation)
            _radiant.call('stonehearth:create_ghost_workbench', workbench_entity, location, rotation, crafter:get_id())
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

--- Create a shadow of the workbench.
--  Workers are not actually asked to create the workbench until the outbox is placed too. 
function WorkshopCallHandler:create_ghost_workbench(session, response, workbench_entity_uri, pt, rotation, crafter_id)
   local location = Point3(pt.x, pt.y, pt.z)
   local workbench = radiant.entities.create_entity(workbench_entity_uri)

   local entity_forms = workbench:get_component('stonehearth:entity_forms')
   local ghost_entity = entity_forms:get_ghost_entity()

   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)

   local crafter = radiant.entities.get_entity(crafter_id)

   local crafter_component = crafter:get_component('stonehearth:crafter')
   if not crafter_component then
      return false
   end
   crafter_component:create_workshop(ghost_entity, location)
   return true
end

--- Destroys a shadow of a workbench
function WorkshopCallHandler:destroy_ghost_workbench(session, response, ghost_entity_id)
   local ghost_entity = radiant.entities.get_entity(ghost_entity_id)
   radiant.entities.destroy_entity(ghost_entity)
end

return WorkshopCallHandler
