-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 
-- xxxxxxxxxxxxxxxxxxx 

-- rename this thing ExecutionSequence, cause it's more  in the framework than not!

local placeholders = require 'services.server.ai.placeholders'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local CompoundAction = class()

function CompoundAction:__init(entity, injecting_entity, action_ctor, activities, when_predicates, think_output_placeholders)
   -- initialize metadata
   self._entity = entity
   self._injecting_entity = injecting_entity
   self._action = action_ctor(entity, injecting_entity)
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
   self._thinking_frames = {}
   self._running_frames = {}
   self._previous_think_output = {}
   self.version = 2
   self._id = stonehearth.ai:get_next_object_id()   
   self._log = radiant.log.create_logger('compound_action')

   -- if the action has start_thinking or stop_thinking methods, create a temporary
   -- ai object we can use to forward the think output to the first execution frame
   -- in the compound action.
   if self._action.start_thinking or self._action.stop_thinking then
      self._action_ai = {
         set_think_output = function(_, think_output)
            self:_spam_current_state('compound action became ready!')
            think_output = think_output or self._args
            table.insert(self._previous_think_output, think_output)
            self:_start_thinking(1)
         end,
         clear_think_output = function(_)
            local msg = string.format('compound action became unready.  aborting')
            self._log:debug(msg)
            self._ai:abort(msg)
         end,
      }
   end
end

function CompoundAction:start_thinking(ai, entity, args)
   assert(#self._thinking_frames == 0)
   assert(#self._previous_think_output == 0)
   assert(not self._thinking)
   
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
   
   self._thinking = true
   self:_start_thinking(0)
end

function CompoundAction:_start_thinking(index)
   assert(self._thinking)

   -- index is the frame in the compound action which needs to start thinking first.  use 0
   -- as a sentinel to refer to the compound action itself. 
   if index == 0 then
      if self._action.start_thinking then
         self:_spam_current_state('start_thinking (compound action)')
         self._action.start_thinking(self._action, self._action_ai, self._entity, self._args)
         return
      else
         index = 1
      end
   end

   -- now ask all the compound actions to start thinking, in order.
   while index <= #self._activities do
      local activity = self._activities[index]
      self:_spam_current_state('start_thinking (%d of %d - %s)', index, #self._activities, activity.name)

      local transformed_args = self:_replace_placeholders(activity.args)
      local frame = self._ai:spawn(activity.name, transformed_args)
      table.insert(self._thinking_frames, frame)

      local think_output = frame:start_thinking(transformed_args, self._ai.CURRENT)
      if think_output then
         self:_spam_current_state('after start_thinking (%d of %d - %s)', index, #self._activities, activity.name)
         assert(self._thinking)
         table.insert(self._previous_think_output, think_output)
         index = index + 1
      else
         self:_spam_current_state('registering on_ready handler (%d of %d - %s)', index, #self._activities, activity.name)
         frame:on_ready(function(frame, think_output)
            if not think_output then
               local msg = string.format('previous frame f:%d became unready.  aborting', frame:get_id())
               self._log:debug(msg)
               self._ai:abort(msg)
            else
               self:_spam_current_state('frame became ready! (%d of %d - %s)', index, #self._activities, activity.name)
            end
            assert(self._thinking, 'received ready callback while not thinking.')
            table.insert(self._previous_think_output, think_output)
            self:_start_thinking(index + 1)
            -- xxx: listen for when frames become unready, right?      
         end)
         return
      end
   end
   self:_set_think_output()   
end

function CompoundAction:_set_think_output()
   assert(self._thinking)
   
   self._log:detail('all frames ready!  calling set_think_output.')
   if self._think_output_placeholders then
      local replaced = self:_replace_placeholders(self._think_output_placeholders)
      self._ai:set_think_output(replaced)
   else
      self._ai:set_think_output()
   end   
end

function CompoundAction:stop_thinking(ai, entity, ...)
   self._thinking = false

   self._previous_think_output = {}
   for i=#self._thinking_frames,1,-1 do
      self._thinking_frames[i]:stop_thinking() -- must be asynchronous!
      self._thinking_frames[i]:on_ready(nil)
   end
   if self._action.stop_thinking then
      self._action.stop_thinking(self._action, self._action_ai, self._entity, self._args)
   end
   self._log:debug('switching thinking frames to running frames')   
   self._running_frames = self._thinking_frames
   self._thinking_frames = {}
end

function CompoundAction:_replace_placeholders(args)
   local replaced  = {}
   for name, value in pairs(args) do
      if type(value) == 'table' and value.__placeholder then
         local result = value:__eval(self)
         if result == nil or result == placeholders.INVALID_PLACEHOLDER then
            error(string.format('placeholder %s failed to return a value in "%s" action', tostring(value), self.name))
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
   self._log:detail('starting all compound action frames')
   for _, frame in ipairs(self._thinking_frames) do
      frame:start() -- must be synchronous!
   end
   self._log:detail('finished starting all compound action frames')
end

function CompoundAction:run(ai, entity, ...)
   for _, frame in ipairs(self._running_frames) do
      frame:run()  -- must be synchronous!
   end
end

function CompoundAction:stop()
   self._log:detail('stopping all compound action frames')
   for _, frame in ipairs(self._running_frames) do
      frame:stop() -- must be asynchronous!
   end
   self._running_frames = {}
   self._log:detail('finished stopping all compound action frames')
end

function CompoundAction:destroy()
   self._log:detail('destroying all compound action frames')
   local frames = (#self._thinking_frames > 0) and self._thinking_frames or self._running_frames
   
   for i = #frames, 1, -1 do
      local frame = frames[i]
      frame:destroy() -- must be synchronous!
   end
   if self._action.destroy then
      self._action:destroy(self._ai, self._entity, self._args)
   end
end

function CompoundAction:get_debug_info()
   local info = {
      name = self.name,
      does = self.does,
      args = stonehearth.ai:format_args(self._args),
      priority = self.priority,
      execution_frames = {
         n = 0,
      }
   }
   local frames = (#self._thinking_frames > 0) and self._thinking_frames or self._running_frames
   for _, f in ipairs(frames) do
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
