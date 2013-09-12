local client_entities = {}
local singleton = {}

-- xxx: could use some factoring with the server entities...

local Point3 = _radiant.csg.Point3

function client_entities.__init()
   singleton._entity_dtors = {}
end

function client_entities.create_entity(arg1, arg2)
   if not arg1 then
      return _client:create_empty_authoring_entity()
   end
   if not arg2 then
      local entity_ref = arg1 -- something like 'entity(stonehearth, wooden_sword)'
      assert(entity_ref:sub(1, 7) == 'entity(')
      radiant.log.info('creating entity %s', entity_ref)
      return _client:create_authoring_entity_by_ref(entity_ref)
   end
   local mod_name = arg1 -- 'stonehearth'
   local entity_name = arg2 -- 'wooden_sword'
   radiant.log.info('creating entity %s, %s', mod_name, entity_name)
   return native:create_authoring_entity(mod_name, entity_name)
end

function client_entities.destroy_entity(entity)
   if entity then
      _client:destroy_authoring_entity(entity)
   end
end

client_entities.__init()
return client_entities
