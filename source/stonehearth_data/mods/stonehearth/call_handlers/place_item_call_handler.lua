local priorities = require('constants').priorities.worker_task
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local PlaceItemCallHandler = class()
local log = radiant.log.create_logger('call_handlers.place_item')

-- Client side object to place an item in the world. The item exists as an icon first
-- This method is invoked by POSTing to the route for this file in the manifest.
-- TODO: merge/factor out with CreateWorkshop?
function PlaceItemCallHandler:choose_place_item_location(session, response, item_to_place)
   --Save whether the entity is just a type or an actual entity to determine if we're 
   --going to place an actual object or just an object type
   assert(item_to_place, "Must pass entity data about the object to place")

   local placement_test_entity, destroy_placement_test_entity, cursor_uri
   local next_call, target_entity_data, test_entity, entity_being_placed, ghost_entity, iconic_entity
   
   if type(item_to_place) == 'string' then   
      next_call = 'stonehearth:place_item_type_in_world'
      placement_test_entity = radiant.entities.create_entity(item_to_place)
      item_to_place = placement_test_entity:get_component('stonehearth:entity_forms')
                                             :get_iconic_entity()
                                                :get_uri()
      destroy_placement_test_entity = true
   else
      next_call = 'stonehearth:place_item_in_world'
     
      -- if this is the iconic version, use the actual ghost for the template
      local iconic_form = item_to_place:get_component('stonehearth:iconic_form')
      if iconic_form then
         placement_test_entity = iconic_form:get_root_entity()
      end
      if not placement_test_entity then
         local ghost_form = item_to_place:get_component('stonehearth:ghost_form')
         if ghost_form then
            placement_test_entity = ghost_form:get_root_entity()
         end
      end
      if not placement_test_entity then
         placement_test_entity = item_to_place
      end
   end
   assert(placement_test_entity, 'could not determine placement test entity')
   local entity_forms = placement_test_entity:get_component('stonehearth:entity_forms')
   if entity_forms then
      iconic_entity = entity_forms:get_iconic_entity()
      ghost_entity = entity_forms:get_ghost_entity()
      if ghost_entity then
         cursor_uri = ghost_entity:get_uri()
      end
   end
   if not cursor_uri then
      cursor_uri = placement_test_entity:get_uri()
   end
      
   stonehearth.selection:select_location()
      :use_ghost_entity_cursor(cursor_uri)
      :set_filter_fn(function (result)
            if result.entity == item_to_place then
               return stonehearth.selection.FILTER_IGNORE
            end
            if result.entity == iconic_entity then
               return stonehearth.selection.FILTER_IGNORE
            end
            if result.entity == ghost_entity then
               return stonehearth.selection.FILTER_IGNORE
            end
            return radiant.terrain.is_standable(placement_test_entity, result.brick)
         end)
      :done(function(selector, location, rotation)
            _radiant.call(next_call, item_to_place, location, rotation)
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
            response:reject('no location')
         end)
      :always(function()
            if destroy_placement_test_entity then
               radiant.entities.destroy_entity(placement_test_entity)
            end
         end)
      :go()
end

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_in_world(session, response, item, location, rotation)
   local location = Point3(location.x, location.y, location.z)
   if not item or not radiant.util.is_a(item, Entity) or not item:is_valid() then
      return false
   end
   local iconic_form = item:get_component('stonehearth:iconic_form')
   if iconic_form then
      item = iconic_form:get_root_entity()      
   end
   local ghost_form = item:get_component('stonehearth:ghost_form')
   if ghost_form then
      item = ghost_form:get_root_entity()      
   end
   
   local entity_forms = item:get_component('stonehearth:entity_forms')
   if not entity_forms then
      response:reject({ error = 'item has no entity_forms component '})
   end
   
   radiant.entities.set_player_id(item, session.player_id)
   entity_forms:place_item_in_world(location, rotation)

   return true
end

--- Place any object that matches the entity_uri
-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_in_world(session, response, entity_uri, location, rotation)
   local location = Point3(location.x, location.y, location.z)

   -- look for entities which are not currently being placed
   local candidates = stonehearth.inventory:get_inventory(session.player_id)
                                           :get_items_of_type(entity_uri)

   -- returns the best item to place.  the best item is the one that isn't currently
   -- being placed and is closest to the placement location
   local best_item, best_distance, more_items
   for _, item in pairs(candidates.items) do
      local position = radiant.entities.get_world_grid_location(item)
      local distance = position:distance_to(location)

      -- make sure the item is better than the previous one.
      if not best_item or distance < best_distance then
         -- make sure the item isn't being placed
         local entity_forms = item:get_component('stonehearth:iconic_form')
                                    :get_root_entity()
                                    :get_component('stonehearth:entity_forms')
         if not entity_forms:is_being_placed() then
            more_items = more_items or best_item ~= nil
            best_item, best_distance = item, distance
         end
      end
   end

   -- place the best item, if it exists
   if not best_item then
      response:fail({error = 'no more placeable items'})
   end
   self:place_item_in_world(session, response, best_item, location, rotation)

   -- return whether or not there are most items we could potentially place
   response:resolve({ more_items = more_items })
end


return PlaceItemCallHandler
