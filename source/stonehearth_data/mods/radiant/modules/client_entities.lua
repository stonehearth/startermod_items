local client_entities = {}
local singleton = {}
local log = radiant.log.create_logger('client')

-- xxx: could use some factoring with the server entities...

local Point3 = _radiant.csg.Point3

function client_entities.__init()
   singleton._entity_dtors = {}
end

function client_entities.create_entity(ref)
   if not ref then
      return _radiant.client.create_empty_authoring_entity()
   end
   log:info('client', 'creating entity %s', ref)
   return _radiant.client.create_authoring_entity(ref)
end

function client_entities.destroy_entity(entity)
   if entity then
      _radiant.client.destroy_authoring_entity(entity:get_id())
   end
end

-- id here can be an int (e.g. 999) or uri (e.g. '/o/stores/server/objects/999')
function client_entities.get_entity(id)
   local entity = _radiant.client.get_object(id)
   return entity
end

function client_entities.get_entity_data(entity, key)
   if entity then
      local uri = entity:get_uri()
      if uri and #uri > 0 then
         local json = radiant.resources.load_json(uri)
         if json.entity_data then
            return json.entity_data[key]
         end
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

client_entities.__init()
return client_entities
