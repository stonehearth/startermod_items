local priorities = require('constants').priorities.worker_task
local Point3 = _radiant.csg.Point3

local PlaceItemCallHandler = class()
local log = radiant.log.create_logger('call_handlers.place_item')


-- Client side object to place an item in the world. The item exists as an icon first
-- This method is invoked by POSTing to the route for this file in the manifest.
-- TODO: merge/factor out with CreateWorkshop?
function PlaceItemCallHandler:choose_place_item_location(session, response, target_entity, entity_uri)
   --Save whether the entity is just a type or an actual entity to determine if we're 
   --going to place an actual object or just an object type
   assert(target_entity, "Must pass entity data about the object to place")
   local next_call, target_entity_data
   if type(target_entity) == 'string' then
      next_call = 'stonehearth:place_item_type_in_world'
      target_entity_data = target_entity
   else
      next_call = 'stonehearth:place_item_in_world'
      target_entity_data = target_entity:get_id()
   end
   
   stonehearth.selection.select_place_location()
      :use_ghost_entity_cursor(entity_uri)
      :done(function(selector, location, rotation)
            _radiant.call(next_call, target_entity_data, entity_uri, location, rotation)
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

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_in_world(session, response, entity_id, full_sized_uri, location, rotation)
   local location = Point3(location.x, location.y, location.z)
   local item = radiant.entities.get_entity(entity_id)
   if not item then
      return false
   end

   local town = stonehearth.town:get_town(session.player_id)
   town:place_item_in_world(item, full_sized_uri, location, rotation)

   return true
end

--- Place any object that matches the entity_uri
-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_in_world(session, response, entity_uri, full_item_uri, location, rotation)
   local location = Point3(location.x, location.y, location.z)

   local town = stonehearth.town:get_town(session.player_id)
   town:place_item_type_in_world(entity_uri, full_item_uri, location, rotation)
   
   return true
end


return PlaceItemCallHandler
