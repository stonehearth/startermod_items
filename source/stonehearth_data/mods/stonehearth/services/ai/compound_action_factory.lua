local CompoundAction = require 'services.ai.compound_action'
local CompoundActionFactory = class()

function CompoundActionFactory:__init(action)
   assert(action)
   self.does = action.does
   self.version = action.version
   self.args = action.args
   self.think_output = action.think_output
   self.priority = action.priority

   self._base_action = action
   self._when_sequence = {}
   self._activity_sequence = {}
   self._think_output_placeholders = nil
end

function CompoundActionFactory:when(fn)
   assert(#self._activity_sequence == 0, 'all when clauses must be placed at the start of a compound action')
   assert(not self._think_output_placeholders, 'all when clauses must be placed at the start of a compound action')
   table.insert(self._when_sequence, fn)
   return self
end

function CompoundActionFactory:execute(name, args)
   assert(not self._think_output_placeholders, 'set_think_output can only be called once, at the end of compound action')

   local activity = stonehearth.ai:create_activity(name, args)
   table.insert(self._activity_sequence, activity)
   return self
end

function CompoundActionFactory:set_think_output(arg_placeholders)
   assert(not self._think_output_placeholders, 'set_think_output can only be called once, at the end of compound action')
   self._think_output_placeholders = arg_placeholders
   return self
end

function CompoundActionFactory:create_action(ai_component, entity, injecting_entity)
   local unit = stonehearth.ai:create_execution_unit(ai_component, nil, self._base_action, entity, injecting_entity)
   return CompoundAction(unit, self._activity_sequence, self._when_sequence, self._think_output_placeholders)
end

return CompoundActionFactory
