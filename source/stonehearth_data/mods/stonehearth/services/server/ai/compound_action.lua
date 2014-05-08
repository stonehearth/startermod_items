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
   self._previous_think_output = {}

   self.version = 2
   self._id = stonehearth.ai:get_next_object_id()   
   self._log = radiant.log.create_logger('compound_action')

   -- if the action has start_thinking or stop_thinking methods, create a temporary
   -- ai object we can use to forward the think output to the first execution frame
   -- in the compound action.
   if self._action.start_thinking or self._action.stop_thinking or self._action.start or self._action.stop then
      self._action_ai = {
         get_log = function() return self._ai:get_log() end,
         set_status_text = function(_, ...)
            self._ai:set_status_text(...)
         end,
         set_think_output = function(_, think_output)
            self:_spam_current_state('compound action became ready!')
            think_output = think_output or self._args
            table.insert(self._previous_think_output, think_output)

            -- verify the compound action only calls set_think_output once at
            -- the beginning of the thinking process
            assert(#self._previous_think_output == 1)
            self:_start_thinking()
         end,
         clear_think_output = function(_)
            local msg = string.format('compound action became unready.  aborting')
            self._log:debug(msg)
            self._ai:abort(msg)
         end,
      }
   end
end

function CompoundAction:_create_execution_frames()
   self._execution_frames = {} 
   for i, activity in ipairs(self._activities) do
      self._log:detail('creating compound action frame #%d : %s', i, activity.name)
      local frame = self._ai:spawn(activity.name)
      table.insert(self._execution_frames, frame)
   end
end

function CompoundAction:_destroy_execution_frames()
   if self._execution_frames then
      local count = #self._execution_frames
      for i = count, 1, -1  do
         self._execution_frames[i]:destroy()
      end
      self._execution_frames = nil
   end
end

function CompoundAction:start_thinking(ai, entity, args)
   assert(not self._thinking)
   assert(#self._previous_think_output == 0)
   
   self._ai = ai
   self._args = args
   self._log = ai:get_log()

   if not self._execution_frames then
      self:_create_execution_frames()
   end

   -- invoke all the when clauses.  if any of them return false, just bail immediately
   for i, clause_fn in ipairs(self._when_predicates) do
      if not clause_fn(ai, entity, args) then
         self._log:debug('when clause %d failed.  hanging in start_thinking intentionally', i)
         return
      end
      self._log:spam('when %d succeeded', i)
   end
 
   -- copy ai.CURRENT into self._action_ai.CURRENT for actions that implement start_thinking()
   if self._action_ai then
      self._action_ai.CURRENT = ai.CURRENT
   end
   
   -- start thinking for real!
   self._thinking = true
   self:_start_thinking()
end

function CompoundAction:_start_thinking()
   assert(self._thinking)

   -- index is the frame in the compound action which needs to start thinking first.  use 0
   -- as a sentinel to refer to the compound action itself. 
   local current_frame = #self._previous_think_output
   if current_frame == 0 then
      if self._action.start_thinking then
         self:_spam_current_state('start_thinking (compound action)')
         self._action:start_thinking(self._action_ai, self._entity, self._args)
         return
      else
         -- the compound action does not implement start_thinking().. hrm.  what should the
         -- result of ai.PREV of the first :execute()'ed action be?  let's just define that
         -- to be args().
         table.insert(self._previous_think_output, self._args)
         current_frame = 1
      end
   end

   -- now ask all the compound actions to start thinking, in order.

   while current_frame <= #self._execution_frames do
      assert(current_frame == #self._previous_think_output)

      local activity = self._activities[current_frame]
      self:_spam_current_state('start_thinking (%d of %d - %s)', current_frame, #self._activities, activity.name)

      local transformed_args = self:_replace_placeholders(activity.args)
      local frame = self._execution_frames[current_frame]
      local think_output = frame:start_thinking(transformed_args, self._ai.CURRENT)

      if think_output then
         self:_spam_current_state('after start_thinking (%d of %d - %s)', current_frame, #self._activities, activity.name)
         table.insert(self._previous_think_output, think_output)
         current_frame = current_frame + 1
      else
         self:_spam_current_state('registering on_ready handler (%d of %d - %s)', current_frame, #self._activities, activity.name)
         frame:on_ready(function(frame, think_output)
            self._log:detail('on_ready callback fired.')
            assert(self._thinking, 'received ready callback while not thinking.')

            if not think_output then
               local msg = string.format('previous frame f:%d became unready.  aborting', frame:get_id())
               self._log:debug(msg)
               self._ai:abort(msg)
               return
            else
               self:_spam_current_state('frame became ready! (%d of %d - %s)', current_frame, #self._activities, activity.name)
            end
            table.insert(self._previous_think_output, think_output)
            self:_start_thinking()
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
   for i=#self._execution_frames, 1, -1 do
      self._execution_frames[i]:stop_thinking() -- must be synchronous!
      self._execution_frames[i]:on_ready(nil)
   end
   
   if self._action.stop_thinking then
      self._action.stop_thinking(self._action, self._action_ai, self._entity, self._args)
   end
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
   
   if self._action.start then
      self._action:start(self._action_ai, self._entity, self._args)
   end

   for _, frame in ipairs(self._execution_frames) do
      frame:start() -- must be synchronous!
   end
   self._log:detail('finished starting all compound action frames')
end

function CompoundAction:run(ai, entity, ...)
   if self._action.status_text then
      self._ai:set_status_text(self._action.status_text)
   end   
   for _, frame in ipairs(self._execution_frames) do
      frame:run()  -- must be synchronous!
   end
end

function CompoundAction:stop()
   self._log:detail('stopping all compound action frames')
   
   if self._action.stop then
      self._action:stop(self._action_ai, self._entity, self._args)
   end

   for _, frame in ipairs(self._execution_frames) do
      frame:stop() -- must be asynchronous!
   end
   self._log:detail('finished stopping all compound action frames')
end

function CompoundAction:destroy()
   self._log:detail('destroying all compound action frames')

   self:_destroy_execution_frames()
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
   if self._execution_frames then
      for _, f in ipairs(self._execution_frames) do
         table.insert(info.execution_frames, f:get_debug_info())
      end
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
