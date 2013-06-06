local AI = class()

function AI:__init(entity)
   self._entity = entity
end

function AI:extend(json)
   radiant.ai._init_entity(self._entity, json)
end

return AI
