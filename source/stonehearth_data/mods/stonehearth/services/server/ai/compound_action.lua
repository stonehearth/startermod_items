-- rename this thing ExecutionSequence, cause it's more  in the framework than not!

local Entity = _radiant.om.Entity

local placeholders = require 'services.server.ai.placeholders'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local CompoundAction = radiant.class()

function CompoundAction:__init(entity, injecting_entity, action_ctor, activities, think_output_placeholders)
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
   self.unprotected_args = true
   self.think_output = self._action.think_output

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
      self.unprotected_args = false
      self._action_ai = {
         get_log = function() return self._ai:get_log() end,
         set_status_text = function(_, ...)
            self._ai:set_status_text(...)
         end,
         set_cost = function(_, cost)
            self._action_cost = cost
         end,
         set_think_output = function(_, think_output)
            self:_spam_current_state('compound action became ready!')
            think_output = think_output or self._args
            table.insert(self._previous_think_output, think_output)

            -- verify the compound action only calls set_think_output once at
            -- the beginning of the thinking process
            assert(#self._previous_think_output == 1)
            self:_start_thinking_on_frame(1)
         end,
         abort = function(_, ...)
            self._ai:abort(...)
         end,
         clear_think_output = function(_)
            if self._thinking then
               self._log:debug('compound action called clear_think_output while thinking.  restarting thinking.')
               self:_restart_thinking()
               return
            end
            local msg = string.format('compound action became unready in non-think state.  aborting')
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
   self:_update_debug_info()
end

function CompoundAction:_destroy_execution_frames()
   if self._execution_frames then
      local count = #self._execution_frames
      for i = count, 1, -1  do
         self._execution_frames[i]:destroy()
      end
      self._execution_frames = nil
   end
   self:_update_debug_info()   
end

function CompoundAction:_restart_thinking()
   assert(self._thinking)  -- we start in the thinking state...

   -- stop thinking on all the frames which may have given us results
   self._previous_think_output = {}
   for i=#self._execution_frames, 1, -1 do
      self._execution_frames[i]:stop_thinking() -- must be synchronous!
      self._execution_frames[i]:set_think_progress_cb(nil)
   end

   -- the compound action itself is already thinking, so there's no reason
   -- to call :start_thinking() on it.
   if not self._action.start_thinking then
      self:_start_thinking_on_frame(1)
   end

   assert(self._thinking)  -- we remain in the thinking state...
end

function CompoundAction:start_thinking(ai, entity, args)
   assert(not self._thinking)
   assert(#self._previous_think_output == 0)

   self._ai = ai
   self._log = ai:get_log()
   self._action_cost = 0
   self:_set_args(args)

   if not self._execution_frames then
      self:_create_execution_frames()
   end

   -- copy ai.CURRENT into self._action_ai.CURRENT for actions that implement start_thinking()
   if self._action_ai then
      self._action_ai.CURRENT = ai.CURRENT
   end
   
   if self._restart_after_halt_timer then
      self._restart_after_halt_timer:destroy()
      self._restart_after_halt_timer = nil
   end

   -- starts the first call to :start_thinking() of everyone who needs to get a result before
   -- we notify the ai that thinking is done.  this is either the registering compound action
   -- itself, or the first execute frame in its chain.
   --
   self._thinking = true
   if self._action.start_thinking then
      self:_spam_current_state('start_thinking (compound action)')
      self._action:start_thinking(self._action_ai, self._entity, self._args)
   else
      self:_start_thinking_on_frame(1)
   end
end

function CompoundAction:_start_thinking_on_frame(i)
   if i > #self._execution_frames then
      self:_spam_current_state('all frames ready!  setting think output')
      self:_set_think_output()
      return      
   end

   local activity = self._activities[i]
   self:_spam_current_state('start_thinking (%d of %d - %s)', i, #self._activities, activity.name)

   local frame = self._execution_frames[i]

   frame:set_think_progress_cb(function(ready_frame, status, think_output)
         self._log:detail('think progress callback fired for %s. (%s)', activity.name, status)
         assert(self._thinking, 'received ready callback while not thinking.')

         if status == 'ready' then
            assert(think_output)
            assert(ready_frame == frame, 'unexpected ready callback (or ready delivered more than once!)')

            self:_spam_current_state('frame became ready! (%d of %d - %s)', i, #self._activities, activity.name)
            table.insert(self._previous_think_output, think_output)
            self:_start_thinking_on_frame(i + 1)
         elseif status == 'unready' then
            local msg = string.format('previous frame f:%d became unready.  aborting', frame:get_id())
            self._log:debug(msg)
            self._ai:abort(msg)
         elseif status == 'halted' then
            self:_restart_after_halt()
         end
      end)

   local transformed_args = self:_replace_placeholders(activity.args)
   frame:start_thinking(transformed_args, self._ai.CURRENT)
end

function CompoundAction:_set_think_output()
   assert(self._thinking)
   
   self._log:detail('all frames ready!  calling set_think_output.')

   -- set the cost of the action.  if the compound action has opt-ed out of accumulating
   -- the cost of it's children by setting Action.fixed_cost to some number, use that.
   -- otherwise, the cost of a compound action is the sum of the result of the :set_cost()
   -- plus the costs of all the actions it needs to execute.
   local cost = self._action.fixed_cost
   if not cost then
      cost = self._action_cost
      for _, frame in ipairs(self._execution_frames) do
         cost = cost + frame:get_cost()
      end
   end
   self._ai:set_cost(cost)

   -- mo comments, yo...
   if self._think_output_placeholders then
      local replaced = self:_replace_placeholders(self._think_output_placeholders)
      self._ai:set_think_output(replaced)
   else
      self._ai:set_think_output()
   end
end

function CompoundAction:_restart_after_halt()   
   assert(self._thinking)
   assert(not self._restart_after_halt_timer)

   self:_spam_current_state('starting restart thinking timer in _restart_after_halt')

   -- allow some time to pass before trying to resume thinking to wait for
   -- initial conditions to change.  how long is long enough?  if we way too long,
   -- the entity will just sort of hang out and look confused.  if we don't wait
   -- long enough we'll end up spinning.  the best case is probably to add
   -- some sort of exponential backoff, but until that's proven, err on the side of
   -- responsiveness. -- tony
   self._restart_after_halt_timer = radiant.set_realtime_timer(20, function()
         self:_spam_current_state('unwinding thinking in all previous frames in _restart_after_halt')
         assert(self._thinking)
         self:stop_thinking()
         self:start_thinking(self._ai, self._entity, self._args)
      end)
end

function CompoundAction:stop_thinking(ai, entity, ...)
   self._thinking = false

   if self._restart_after_halt_timer then
      self._restart_after_halt_timer:destroy()
      self._restart_after_halt_timer = nil
   end

   self._previous_think_output = {}
   for i=#self._execution_frames, 1, -1 do
      self._execution_frames[i]:stop_thinking() -- must be synchronous!
      self._execution_frames[i]:set_think_progress_cb(nil)
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

function CompoundAction:start(ai, entity, args)
   self._log:detail('starting all compound action frames')
   
   if self._action.start then
      self._action:start(self._action_ai, self._entity, self._args)
   end

   for _, frame in ipairs(self._execution_frames) do
      frame:start() -- must be synchronous!
   end   
   self._log:detail('finished starting all compound action frames')

   for _, obj in pairs(self._args) do
      -- unprotect the arguments when starting a compound action
      -- the individual sub-actions will protect their own arguments and we don't want to fail
      -- if the sub-action is supposed to destroy the entity
      ai:unprotect_argument(obj)
   end      
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

   -- stop execution frames in the reverse order.  entities tend to be
   -- created and passed downstream (e.g. create a proxy entity, then find
   -- a path to it).  if we stopped actions in-order, the create proxy action
   -- would destroy the entity before the pathfinder had an opportunity to kill
   -- itself, resulting in an protected entity destroyed error.
   local c = #self._execution_frames
   for i =c,1,-1 do
      self._execution_frames[i]:stop() -- must be asynchronous!
   end
   self._log:detail('finished stopping all compound action frames')
end

function CompoundAction:destroy()
   self._log:detail('destroying all compound action frames')

   self:_destroy_execution_frames()
   if self._action.destroy then
      self._action:destroy(self._ai, self._entity, self._args)
   end
   radiant.log.return_logger(self._log)
   self._log = nil
end

function CompoundAction:get_debug_info(depth)
   if not self._debug_info then
      self._debug_info = radiant.create_datastore({ depth = depth + 1})
      self:_update_debug_info()
   end
   return self._debug_info
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
   if self._log:is_enabled(radiant.log.SPAM) then
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
end

function CompoundAction:_set_args(args)
   self._args = args
   if self._debug_info then
      self._debug_info:modify(function(o)
            args = stonehearth.ai:format_args(self._args)
         end)
   end
end

function CompoundAction:_update_debug_info(args)
   if self._debug_info then
      self._debug_info:modify(function(o)
            o.name = self.name
            o.does = self.does
            o.args = stonehearth.ai:format_args(self._args)
            o.priority = self.priority
            o.execution_frames = {
               n = 0,
            }
            if self._execution_frames then
               for _, f in ipairs(self._execution_frames) do
                  table.insert(o.execution_frames, f:get_debug_info(o.depth))
               end
            end
         end)
   end
end

return CompoundAction
