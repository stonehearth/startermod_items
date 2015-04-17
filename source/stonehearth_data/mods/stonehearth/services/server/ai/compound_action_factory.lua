local CompoundAction = require 'services.server.ai.compound_action'
local CompoundActionFactory = class()

function CompoundActionFactory:__init(action)
   assert(action)
   self.realtime = action.realtime or false
   self.does = action.does
   self.version = action.version
   self.args = action.args
   self.think_output = action.think_output
   self.priority = action.priority

   self._base_action = action
   self._activity_sequence = {}
   self._think_output_placeholders = nil
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

function CompoundActionFactory:create_action(entity, injecting_entity)
   return CompoundAction(entity, injecting_entity, self._base_action, self._activity_sequence, self._think_output_placeholders)
end

return CompoundActionFactory
