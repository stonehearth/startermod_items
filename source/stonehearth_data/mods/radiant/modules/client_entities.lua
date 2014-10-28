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

client_entities.__init()
return client_entities
