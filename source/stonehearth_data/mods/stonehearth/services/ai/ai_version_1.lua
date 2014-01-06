local AiVersion1 = class()
local log = radiant.log.create_logger('ai.version1')

-- injecting_entity is optional
function AiVersion1:__init(ai_component)
   self._ai_component = ai_component
end

function AiVersion1:set_action(action)
   self._action = action
end

function AiVersion1:set_action_priority(action, n)
   self._ai_component:set_action_priority(action, n)
end

return AiVersion1
