local build_util = require 'lib.build_util'
local priorities = require('constants').priorities.worker_task
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local PlaceItemCallHandler = class()
local log = radiant.log.create_logger('call_handlers.place_item')

-- these are quite annoying.  we can get rid of them by implementing and using
-- LuaToProto <-> ProtoToLua in the RPC layer (see lib/typeinfo/dispatcher.h)
local function ToPoint3(pt)
   return Point3(pt.x, pt.y, pt.z)
end

-- Client side object to place an item in the world. The item exists as an icon first
-- This method is invoked by POSTing to the route for this file in the manifest.
-- TODO: merge/factor out with CreateWorkshop?
function PlaceItemCallHandler:choose_place_item_location(session, response, item_to_place)
   --Save whether the entity is just a type or an actual entity to determine if we're 
   --going to place an actual object or just an object type
   assert(item_to_place, "Must pass entity data about the object to place")

   local placement_test_entity, destroy_placement_test_entity, cursor_uri
   local target_entity_data, test_entity, entity_being_placed, ghost_entity, iconic_entity
   local specific_item_to_place, item_uri_to_place
   if type(item_to_place) == 'string' then   
      item_uri_to_place = item_to_place
      placement_test_entity = radiant.entities.create_entity(item_to_place)
      --[[  xxx, place_item_type_in_world is capable of dealing with non-iconic entity forms, so this is unnecessary.
      item_to_place = placement_test_entity:get_component('stonehearth:entity_forms')
                                             :get_iconic_entity()
                                                :get_uri()
      ]]
      destroy_placement_test_entity = true
   else
      specific_item_to_place = item_to_place      
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

   -- don't allow rotation if we're placing stuff on the wall
   local rotation_disabled = entity_forms:is_placeable_on_wall()

   local placing_on_wall, placing_on_wall_normal
   stonehearth.selection:select_location()
      :use_ghost_entity_cursor(cursor_uri)
      :set_rotation_disabled(rotation_disabled)
      :set_filter_fn(function (result, selector)
            -- if the entity is any of the forms of the thing we want to place, bail
            if result.entity == item_to_place then
               return stonehearth.selection.FILTER_IGNORE
            end
            if result.entity == iconic_entity then
               return stonehearth.selection.FILTER_IGNORE
            end
            if result.entity == ghost_entity then
               return stonehearth.selection.FILTER_IGNORE
            end
            local normal = result.normal:to_int()
            
            -- check for ground placement
            if entity_forms:is_placeable_on_ground() and normal.y == 1 then
               return radiant.terrain.is_standable(selector:get_cursor_entity(), result.brick)
            end

            -- check for wall placement
            if entity_forms:is_placeable_on_wall() and normal.y == 0 then
               if not radiant.terrain.is_blocked(selector:get_cursor_entity(), result.brick) then
                  local entities = radiant.terrain.get_entities_at_point(result.brick - normal)
                  for _, entity in pairs(entities) do
                     local wall = entity:get_component('stonehearth:wall')
                     if wall then
                        local rotation = build_util.normal_to_rotation(normal)
                        placing_on_wall = entity
                        placing_on_wall_normal = normal
                        selector:set_rotation(rotation)
                        return true
                     end
                  end
               end
            end
         end)
      :done(function(selector, location, rotation)
            local deferred
            if placing_on_wall then
               if specific_item_to_place then
                  deferred = _radiant.call('stonehearth:place_item_on_wall', specific_item_to_place, location, placing_on_wall, placing_on_wall_normal)
               else
                  deferred = _radiant.call('stonehearth:place_item_type_on_wall', item_uri_to_place, location, placing_on_wall, placing_on_wall_normal)
               end
            else
               if specific_item_to_place then
                  deferred = _radiant.call('stonehearth:place_item_in_world', specific_item_to_place, location, rotation)
               else
                  deferred = _radiant.call('stonehearth:place_item_type_in_world', item_uri_to_place, location, rotation)
               end
            end
            deferred
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

local function get_root_entity(item)
   if not item or not radiant.util.is_a(item, Entity) or not item:is_valid() then
      return
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
   if entity_forms then
      return item, entity_forms
   end
end

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_in_world(session, response, item_to_place, location, rotation)
   location = ToPoint3(location)

   local item, entity_forms = get_root_entity(item_to_place)
   if not entity_forms then
      response:reject({ error = 'item has no entity_forms component'})
      return
   end
   
   radiant.entities.set_player_id(item, session.player_id)
   entity_forms:place_item_on_ground(location, rotation)

   return true
end

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_on_wall(session, response, item, location, wall, normal)
   location = ToPoint3(location)
   normal = ToPoint3(normal)

   local item, entity_forms = get_root_entity(item)
   if not entity_forms then
      response:reject({ error = 'item has no entity_forms component'})
   end
   
   radiant.entities.set_player_id(item, session.player_id)
   entity_forms:place_item_on_wall(location, wall, normal)
   return true
end


--- Place any object that matches the entity_uri
-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_in_world(session, response, entity_uri, location, rotation)
   location = Point3(location.x, location.y, location.z)

   --- xxx: make a "place on ground" fixture fabricator type paradigm here

   -- look for entities which are not currently being placed
   local uri = entity_uri
   local data = radiant.entities.get_component_data(uri, 'stonehearth:entity_forms')
   if data and data.iconic_form then
      uri = data.iconic_form
   end
   local item, acceptable_item_count = stonehearth.inventory:get_inventory(session.player_id)
                                                            :find_closest_unused_placable_item(uri, location)

   if item then
      self:place_item_in_world(session, response, item, location, rotation)

      -- return whether or not there are most items we could potentially place
      response:resolve({ 
            more_items = acceptable_item_count > 1,
            item_uri = entity_uri
         })
   else 
      response:reject({error = 'no more placeable items'})
   end
end

--- Place any object that matches the entity_uri
-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_on_wall(session, response, entity_uri, location, wall, normal)
   location = Point3(location.x, location.y, location.z)
   normal = ToPoint3(normal)

   local fixture_uri = entity_uri
   local data = radiant.entities.get_component_data(fixture_uri, 'stonehearth:entity_forms')
   assert(data, 'must send entity-forms root entity to place_item_type_on_wall')
   location = location - radiant.entities.get_world_grid_location(wall)
   stonehearth.build:add_fixture(wall, fixture_uri, location, normal)
   return true
end

function PlaceItemCallHandler:undeploy_item(session, response, item)
   local efc = item:get_component('stonehearth:entity_forms')
   if efc then
      local current_value = efc:get_should_restock()
      efc:set_should_restock(not current_value)
   end   

   return true;
end


return PlaceItemCallHandler
