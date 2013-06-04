local AI = class()

function AI:__init(entity)
end

function AI:extend(entity, json)
   radiant.ai._init_entity(entity, json)
end

return AI
