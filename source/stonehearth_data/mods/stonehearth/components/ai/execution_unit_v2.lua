local ExecutionUnitV2 = class()
local log = radiant.log.create_logger('ai.component')

function ExecutionUnitV2:__init(ai_component, entity, injecting_entity)
   self._entity = entity
   self._ai_component = ai_component
   assert(false)
end

return ExecutionUnitV2
