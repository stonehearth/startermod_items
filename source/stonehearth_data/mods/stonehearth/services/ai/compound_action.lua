local placeholders = require 'services.ai.placeholders'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local CompoundAction = class()

function CompoundAction:__init(action, activities, when_predicates, think_output_placeholders)
   -- initialize metadata
   self._action = action -- just the header, really
   self.name = self._action.name
   self.does = self._action.does
   self.priority = self._action.priority
   self.weight = self._action.weight
   self.name = self._action.name
   self.args = self._action.args
   self.think_output = self._action.think_output

   self._when_predicates = when_predicates
   self._activities = activities
   self._think_output_placeholders = think_output_placeholders
   self._spawned_frames = {}
   self._previous_think_output = {}
   self.version = 2
   self._id = stonehearth.ai:get_next_object_id()   
end

function CompoundAction:start_thinking(ai, entity, args)
   assert(#self._spawned_frames == 0)
   assert(#self._previous_think_output == 0)
   
   self._ai = ai
   self._args = args
   self._log = ai:get_log()   
   for i, clause_fn in ipairs(self._when_predicates) do
      if not clause_fn(ai, entity, args) then
         self._log:debug('when clause %d failed.  hanging in start_thinking intentionally', i)
         return
      end
      self._log:spam('when %d succeeded', i)
   end
   
   self:_start_thinking(1)
end

function CompoundAction:_start_thinking(index)
   assert(not self._thinking)

   self._thinking = true
   if index > #self._activities then
      self:_set_think_output()
      return
   end
   local activity = self._activities[index]
   self:_spam_current_state('start_thinking (%d of %d - %s)', index, #self._activities, activity.name)

   local replaced = self:_replace_placeholders(activity.args)
   local frame = self._ai:spawn(activity.name, replaced)
   table.insert(self._spawned_frames, frame)

   -- xxx: listen for when frames become unready, right?      
   frame:start_thinking(self._ai.CURRENT, function(think_output)
      self:_spam_current_state('after start_thinking (%d of %d - %s)', index, #self._activities, activity.name)
      assert(self._thinking)
      table.insert(self._previous_think_output, think_output)
      self:_start_thinking(index + 1)
   end)
end

function CompoundAction:_set_think_output()
   self._thinking = false
   if self._think_output_placeholders then
      local replaced = self:_replace_placeholders(self._think_output_placeholders)
      ai:set_think_output(replaced)
   else
      ai:set_think_output()
   end   
end

function CompoundAction:stop_thinking(ai, entity, ...)
   for i=#self._spawned_frames,1,-1 do
      self._spawned_frames[i]:stop_thinking()
   end  
end

function CompoundAction:_replace_placeholders(args)
   local replaced  = {}
   for name, value in pairs(args) do
      if type(value) == 'table' and value.__placeholder then
         local result = value:__eval(self)
         if result == nil or result == placeholders.INVALID_PLACEHOLDER then
            self._ai:abort('placeholder %s failed to return a value in %s', tostring(value), self.name)
         end
         replaced[name] = result
      elseif type(value) == 'table' and not radiant.util.is_instance(value) and not radiant.util.is_class(value) then
         replaced[name] = self:_replace_placeholders(value)
      else
         replaced[name] = value
      end
   end
   return replaced
end

function CompoundAction:start()
   for _, frame in ipairs(self._spawned_frames) do
      frame:start()
   end
end

function CompoundAction:run(ai, entity, ...)
   for _, frame in ipairs(self._spawned_frames) do
      frame:run()
   end
end

function CompoundAction:stop()
   for i = #self._spawned_frames, 1, -1 do
      self._spawned_frames[i]:stop()
   end
end

function CompoundAction:destroy()
   for i = #self._spawned_frames, 1, -1 do
      self._spawned_frames[i]:terminate()
   end
end

function CompoundAction:get_debug_info()
   local info = {
      name = self.name,
      does = self.does,
      args = stonehearth.ai:format_args(self._args),
      priority = self.priority,
      execution_frames = {}
   }
   for _, f in ipairs(self._spawned_frames) do
      table.insert(info.execution_frames, f:get_debug_info())
   end
   return info
end

function CompoundAction:_get_args()
   return self._args
end

function CompoundAction:_get_previous_think_output(rewind_offset)
   local index = #self._previous_think_output - rewind_offset + 1
   return self._previous_think_output[index]
end

function CompoundAction:_get_current_entity_state()
   return self._ai.CURRENT
end

function CompoundAction:_get_entity()
   return self._entity
end

function CompoundAction:_spam_current_state(format, ...)
   self._log:spam(string.format(format, ...))
   if self._ai.CURRENT then
      self._log:spam('  CURRENT is %s', tostring(self._ai.CURRENT))
      for key, value in pairs(self._ai.CURRENT) do      
         self._log:spam('  CURRENT.%s = %s', key, tostring(value))
      end   
   else
      self._log:spam('  no CURRENT state!')
   end
end


return CompoundAction
