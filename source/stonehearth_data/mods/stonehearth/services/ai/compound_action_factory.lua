local CompoundAction = require 'services.ai.compound_action'
local CompoundActionFactory = class()

function CompoundActionFactory:__init(action)
   self.does = action.does
   self.version = action.version
   self.args = action.args
   self.run_args = action.run_args
   self.priority = action.priority

   self._base_action = action
   self._activity_sequence = {}
end

function CompoundActionFactory:execute(name, args)
   local activity = {
      name = name,
      args = args or {}
   }
   table.insert(self._activity_sequence, activity)
   return self
end

function CompoundActionFactory:create_action(ai_component, entity, injecting_entity)
   local unit = stonehearth.ai:create_execution_unit(ai_component, nil, self._base_action, entity, injecting_entity)
   return CompoundAction(unit, self._activity_sequence)
end

return CompoundActionFactory
