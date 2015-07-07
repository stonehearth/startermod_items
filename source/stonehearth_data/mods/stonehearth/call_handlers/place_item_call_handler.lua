local build_util = require 'lib.build_util'
local entity_forms = require 'lib.entity_forms.entity_forms_lib'
local priorities = require('constants').priorities.worker_task
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local PlaceItemCallHandler = class()
local log = radiant.log.create_logger('call_handlers.place_item')

-- these are quite annoying.  we can get rid of them by implementing and using
-- LuaToProto <-> ProtoToLua in the RPC layer (see lib/typeinfo/dispatcher.h)
local function ToPoint3(pt)
   return pt and Point3(pt.x, pt.y, pt.z) or nil
end

-- item_to_place may be an entity instance or uri
function PlaceItemCallHandler:choose_place_item_location(session, response, item_to_place)
   assert(item_to_place)

   local specific_item_to_place = nil
   local item_uri_to_place, cursor_uri
   local root_form, ghost_entity, iconic_entity
   local forms_to_ignore = {}

   if type(item_to_place) == 'string' then   
      item_uri_to_place = item_to_place
   else
      specific_item_to_place = item_to_place
      root_form = radiant.entities.get_root_form(specific_item_to_place)
      item_uri_to_place = root_form:get_uri()
   end

   local placement_test_entity = radiant.entities.create_entity(item_uri_to_place)
   assert(placement_test_entity, 'could not determine placement test entity')

   if not root_form then
      -- we should only be able to place entities in their root form
      assert(placement_test_entity == radiant.entities.get_root_form(placement_test_entity))
      root_form = placement_test_entity
   end
   local entity_forms = root_form:get_component('stonehearth:entity_forms')

   if specific_item_to_place then
      forms_to_ignore[root_form:get_id()] = true

      if entity_forms then
         -- used to avoid collisions with existing item that is being moved
         iconic_entity = entity_forms:get_iconic_entity()
         ghost_entity = entity_forms:get_ghost_entity()
         forms_to_ignore[iconic_entity:get_id()] = true
         forms_to_ignore[ghost_entity:get_id()] = true
      end
   end

   cursor_uri = ghost_entity and ghost_entity:get_uri() or placement_test_entity:get_uri()

   -- don't allow rotation if we're placing stuff on the wall
   local rotation_disabled = entity_forms:is_placeable_on_wall()

   local placement_structure, placement_structure_normal
   stonehearth.selection:select_location()
      :use_ghost_entity_cursor(cursor_uri)
      :set_rotation_disabled(rotation_disabled)
      :set_filter_fn(function (result, selector)
            local entity = result.entity

            -- if the entity is any of the forms of the thing we want to place, ignore it
            if forms_to_ignore[entity:get_id()] then
               return stonehearth.selection.FILTER_IGNORE
            end

            local normal = result.normal:to_int()
            local location = result.brick:to_int()

            local designation = radiant.entities.get_entity_data(entity, 'stonehearth:designation')
            if designation then
               if designation.allow_placed_items then
                  return stonehearth.selection.FILTER_IGNORE
               else
                  return false
               end
            end

            local rotation = selector:get_rotation()
            radiant.entities.turn_to(placement_test_entity, rotation)

            -- if the space occupied by the cursor is blocked, we can't place the item there
            local blocking_entities = radiant.terrain.get_blocking_entities(placement_test_entity, location)
            for _, blocking_entity in pairs(blocking_entities) do
               local ignore = forms_to_ignore[blocking_entity:get_id()]
               if not ignore then
                  return false
               end
            end

            -- TODO: prohibit placement when placing over other ghost entities'
            -- root form's solid collision region

            local wall_ok = entity_forms:is_placeable_on_wall() and normal.y == 0 
            local ground_ok = entity_forms:is_placeable_on_ground() and normal.y == 1

            placement_structure, placement_structure_normal = nil

            -- trawl through the list of entities which would potentially support the
            -- fixture and see if there's a good match
            local entities = radiant.terrain.get_entities_at_point(location - normal)
            for _, entity in pairs(entities) do
               if wall_ok then
                  local wall = entity:get_component('stonehearth:construction_progress')
                  if wall then
                     local rotation = build_util.normal_to_rotation(normal)
                     placement_structure = entity
                     placement_structure_normal = normal
                     selector:set_rotation(rotation)
                     return true
                  end
                  if radiant.entities.is_solid_entity(entity) then
                     local rotation = build_util.normal_to_rotation(normal)
                     placement_structure_normal = normal
                     selector:set_rotation(rotation)
                     return true
                  end
               end
               if ground_ok then
                  if entity:get_component('stonehearth:construction_progress') then
                     placement_structure = entity
                     placement_structure_normal = normal
                     return true
                  end
               end
            end
              
            if ground_ok and radiant.terrain.is_supported(placement_test_entity, location) then
               -- we're unblocked and supported so we're good!
               return true
            end

            -- Uncomment when needed to ignore mining zones, water, etc.
            -- local rcs = entity:get_component('region_collision_shape')
            -- if rcs and rcs:get_region_collision_type() == _radiant.om.RegionCollisionShape.NONE then
            --    return stonehearth.selection.FILTER_IGNORE
            -- end

            return stonehearth.selection.FILTER_IGNORE
         end)
      :done(function(selector, location, rotation)
            local deferred
            if placement_structure then
               if specific_item_to_place then
                  deferred = _radiant.call('stonehearth:place_item_on_structure', specific_item_to_place, location, rotation, placement_structure, placement_structure_normal)
               else
                  deferred = _radiant.call('stonehearth:place_item_type_on_structure', item_uri_to_place, location, rotation, placement_structure, placement_structure_normal)
               end
            else
               if specific_item_to_place then
                  deferred = _radiant.call('stonehearth:place_item_in_world', specific_item_to_place, location, rotation, placement_structure_normal)
               else
                  deferred = _radiant.call('stonehearth:place_item_type_in_world', item_uri_to_place, location, rotation, placement_structure_normal)
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
            radiant.entities.destroy_entity(placement_test_entity)
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
function PlaceItemCallHandler:place_item_in_world(session, response, item_to_place, location, rotation, normal)
   location = ToPoint3(location)
   normal = ToPoint3(normal)

   local item, entity_forms = get_root_entity(item_to_place)
   if not entity_forms then
      response:reject({ error = 'item has no entity_forms component'})
      return
   end
   
   radiant.entities.set_player_id(item, session.player_id)
   entity_forms:place_item_on_ground(location, rotation, normal)

   return true
end

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_on_structure(session, response, item, location, rotation, structure_entity, normal)
   location = ToPoint3(location)
   normal = ToPoint3(normal)

   local item, entity_forms = get_root_entity(item)
   if not entity_forms then
      response:reject({ error = 'item has no entity_forms component'})
   end
   
   location = location - radiant.entities.get_world_grid_location(structure_entity)
   stonehearth.build:add_fixture(structure_entity, item, location, normal, rotation)

   return true
end

--- Place any object that matches the entity_uri
-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_in_world(session, response, entity_uri, location, rotation, normal)
   location = ToPoint3(location)
   normal = ToPoint3(normal)

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
      self:place_item_in_world(session, response, item, location, rotation, normal)

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
function PlaceItemCallHandler:place_item_type_on_structure(session, response, entity_uri, location, rotation, structure_entity, normal)
   location = Point3(location.x, location.y, location.z)
   normal = ToPoint3(normal)

   local fixture_uri = entity_uri
   local data = radiant.entities.get_component_data(fixture_uri, 'stonehearth:entity_forms')
   assert(data, 'must send entity-forms root entity to place_item_type_on_structure')
   location = location - radiant.entities.get_world_grid_location(structure_entity)
   stonehearth.build:add_fixture(structure_entity, fixture_uri, location, normal, rotation)
   return true
end

function PlaceItemCallHandler:undeploy_item(session, response, item)
   local efc = item:get_component('stonehearth:entity_forms')
   if efc then
      local current_value = efc:get_should_restock()
      efc:set_should_restock(not current_value)
   end   

   return true
end

return PlaceItemCallHandler
