local Damage = class()
--local AiInjector = require 'services.ai.ai_injector'

function Damage:__init(combat_service, source_entity, target_entity)
   self._combat_service = combat_service
   self._source_entity = source_entity
   self._target_entity = target_entity
   self._damage_sources = {}
end

function Damage:add_damage_source(damage_source)
   table.insert(self._damage_sources, damage_source)
   return self;   
end

function Damage:resolve()
   self._combat_service:apply_damage(self._source_entity, self._target_entity, self)
   return self;   
end

return Damage
