local client_entities = {}
local singleton = {}

-- xxx: could use some factoring with the server entities...

local Point3 = _radiant.csg.Point3

function client_entities.__init()
   singleton._entity_dtors = {}
end

function client_entities.create_entity(ref)
   if not ref then
      return _radiant.client.create_empty_authoring_entity()
   end
   radiant.log.info('creating entity %s', ref)
   return _radiant.client.create_authoring_entity(ref)
end

function client_entities.destroy_entity(entity)
   if entity then
      _radiant.client.destroy_authoring_entity(entity)
   end
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
client_entities.__init()
return client_entities
