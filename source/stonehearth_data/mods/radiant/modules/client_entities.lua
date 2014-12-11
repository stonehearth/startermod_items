local FilteredTrace = require 'modules.filtered_trace'
local Point3 = _radiant.csg.Point3
 
local client_entities = {}
local singleton = {}
local log = radiant.log.create_logger('client')

-- xxx: could use some factoring with the server entities...

function client_entities.__init()
   singleton._entity_dtors = {}
end

function client_entities.create_entity(ref)
   log:info('client', 'creating entity %s', tostring(ref))
   return _radiant.client.create_authoring_entity(ref or "")
end

function client_entities.destroy_entity(entity)
   if entity and entity:is_valid() then
      -- destroy all the children when destorying the parent.  should we do
      -- this from c++?  if not, entities which get destroyed from the cpp
      -- layer won't get this behavior.  maybe that's just impossible (i.e. forbid
      -- it from happening, since the cpp layer knowns nothing about game logic?)
      local ec = entity:get_component('entity_container')
      if ec then
         -- Copy the blueprint's (container's) children into a local var first, because
         -- _set_teardown_recursive could cause the entity container to be invalidated.
         local ec_children = {}
         for id, child in ec:each_child() do
            ec_children[id] = child
         end         
         for id, child in pairs(ec_children) do
            client_entities.destroy_entity(child)
         end
      end
      --If we're the big one, destroy the little and ghost one
      local entity_forms = entity:get_component('stonehearth:entity_forms')
      if entity_forms then
         local iconic_entity = entity_forms:get_iconic_entity()
         if iconic_entity then
             _radiant.client.destroy_authoring_entity(iconic_entity:get_id())
         end
         local ghost_entity = entity_forms:get_ghost_entity()
         if ghost_entity then
             _radiant.client.destroy_authoring_entity(ghost_entity:get_id())
         end
      end
      --If we're the little one, call destroy on the big one and exit
      local iconic_component = entity:get_component('stonehearth:iconic_form')
      if iconic_component then
         local full_sized = iconic_component:get_root_entity()
         client_entities.destroy_entity(full_sized)
         return 
      end

      _radiant.client.destroy_authoring_entity(entity:get_id())
   end
end

-- id here can be an int (e.g. 999) or uri (e.g. '/o/stores/server/objects/999')
function client_entities.get_entity(id)
   local entity = _radiant.client.get_object(id)
   return entity
end

function client_entities.get_component_data(arg0, key)
   local uri
   if type(arg0) == 'string' then
      uri = arg0
   else
      local entity = arg0
      uri = entity:get_uri()
   end
   if uri and #uri > 0 then
      local json = radiant.resources.load_json(uri)
      if json.components then
         return json.components[key]
      end
   end
end

function client_entities.get_entity_data(arg0, key)
   local uri
   if type(arg0) == 'string' then
      uri = arg0
   else
      local entity = arg0
      uri = entity:get_uri()
   end
   if uri and #uri > 0 then
      local json = radiant.resources.load_json(uri)
      if json.entity_data then
         return json.entity_data[key]
      end
   end
end

function client_entities.get_world_grid_location(entity)
   radiant.check.is_entity(entity)
   return entity:add_component('mob'):get_world_grid_location()
end

function client_entities.get_location_aligned(entity)
   radiant.check.is_entity(entity)
   if entity then
      return entity:add_component('mob'):get_grid_location()
   end
end

function client_entities.trace_grid_location(entity, reason)
   local last_location = client_entities.get_world_grid_location(entity)
   local filter_fn = function()
         local current_location = client_entities.get_world_grid_location(entity)

         if current_location == last_location then
            return false
         else 
            last_location = current_location
            return true
         end
      end

   local trace_impl = entity:add_component('mob'):trace_transform(reason)
   local filtered_trace = FilteredTrace(trace_impl, filter_fn)
   return filtered_trace
end

function client_entities.get_player_id(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_player_id() or nil
end

function client_entities.move_to(entity, location)
   radiant.check.is_entity(entity)

   if type(location) == "table" then
      location = Point3(location.x, location.y, location.z)
   end
   entity:add_component('mob'):set_location_grid_aligned(location)
end

function client_entities.turn_to(entity, degrees)
   radiant.check.is_entity(entity)
   radiant.check.is_number(degrees)
   entity:add_component('mob'):turn_to(degrees)
end

function client_entities.add_child(parent, child, location)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:add_component('entity_container')

   component:add_child(child)
   if location then
      client_entities.move_to(child, location)
   else
   end
end

function client_entities.remove_child(parent, child)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:get_component('entity_container')
   if component then
      component:remove_child(child:get_id())
   end
end

function client_entities.get_player_id(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_player_id() or nil
end

function client_entities.local_to_world(pt, e)
   return _radiant.physics.local_to_world(pt, e)
end

function client_entities.world_to_local(pt, e)
   return _radiant.physics.world_to_local(pt, e)
end


function client_entities.is_solid_entity(entity)
   if not entity or not entity:is_valid() then
      return false
   end
   
   if entity:get_id() == 1 then
      return true
   end

   local rcs = entity:get_component('region_collision_shape')
   if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
      return true
   end

   return false
end

function client_entities.get_parent(entity)
   local mob = entity:get_component('mob')
   return mob and mob:get_parent()
end

function client_entities.is_child_of(entity, query_parent)
   assert(query_parent)
  
   local parent = client_entities.get_parent(entity)
   while parent and parent ~= query_parent do
      parent = client_entities.get_parent(parent)
   end
   return parent == query_parent
end


client_entities.__init()
return client_entities
