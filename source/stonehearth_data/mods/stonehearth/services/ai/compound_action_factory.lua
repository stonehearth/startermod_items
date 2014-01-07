local CompoundAction = require 'services.ai.compound_action'
local CompoundActionFactory = class()

function CompoundActionFactory:__init(action)
   self.does = action.does
   self.version = action.version
   self.args = action.args
   self.run_args = action.run_args
   self.priority = action.priority

   self._base_action = action
   self._chained_actions = {}
end

function CompoundActionFactory:execute(...)
   table.insert(self._chained_actions, {...})
   return self
end

function CompoundActionFactory:create_action(ai_component, entity, injecting_entity)
   local unit = stonehearth.ai:create_execution_unit(ai_component, nil, self._base_action, entity, injecting_entity)
   return CompoundAction(unit, self._chained_actions)
end

return CompoundActionFactory
