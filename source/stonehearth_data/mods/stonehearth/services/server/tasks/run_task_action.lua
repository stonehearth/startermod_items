local RunTaskAction = class()

RunTaskAction.version = 2

function RunTaskAction:__init(task, activity)
   self._id = stonehearth.ai:get_next_object_id()
   self._task = task
   self._activity = activity
end

function RunTaskAction:get_entity()
   return self._entity
end

function RunTaskAction:get_log()
   return self._log
end

function RunTaskAction:_create_execution_frame(ai)
   if not self._execution_frame then
      self._ai = ai
      self._log = ai:get_log()

      self._log:detail('creating execution frame for %s', self._activity.name)
      self._execution_frame = ai:spawn(self._activity.name)
      self._started_listener = radiant.events.listen(self._task, 'started', self, self._start_stop_thinking)
      self._stopped_listener = radiant.events.listen(self._task, 'stopped', self, self._start_stop_thinking)
      self._work_available_listener = radiant.events.listen(self._task, 'work_available', self, self._start_stop_thinking)

      if self._debug_info then
         self._debug_info:modify(function(o)
               o.execution_frames = { self._execution_frame:get_debug_info(o.depth) }
            end)
      end
   end
end

function RunTaskAction:_start_stop_thinking()
   if not self._starting then
      local should_think = self._should_think and self._task:__action_try_start_thinking(self)
      self._log:debug('_start_stop_thinking (should? %s  currently? %s)', tostring(should_think), tostring(self._thinking))
      if should_think and not self._thinking then
         self._thinking = true
         local think_output = self._execution_frame:start_thinking(self._activity.args, self._ai.CURRENT)
         if think_output then
            -- hmmm.  we need to set the thing output upward toward our dispatcher.  this needs
            -- to match the expected type.  we have *no way* of knowing that type, though (at least
            -- not currently...).  luckily, set_think_output() with no parameters will just return
            -- the args, which is probably what we want, anyway.  much ado about nothing, i think.
            --
            --    self._ai:set_think_output(think_output) -- this one's wrong...
            --
            self._log:debug('execution frame was ready immediately!  calling set_think_output.')
            self._ai:set_cost(self._execution_frame:get_cost())
            self._ai:set_think_output()
         else
            self._log:debug('execution frame was not ready immediately!  registering think progress handler.')
            self._execution_frame:set_think_progress_cb(function(frame, state, think_output)
               if state == 'ready' then
                  self._log:debug('received ready notification from injected action. (should_think: %s', tostring(self._should_think))
                  self._ai:set_cost(self._execution_frame:get_cost())
                  self._ai:set_think_output()
               elseif state == 'unready' then
                  self._log:debug('received unready notification from injected action.')
                  self._ai:clear_think_output()
               elseif state == 'halted' then
                  self._log:debug('received halt notification from injected action.')
               else
                  assert(false, string.format('unknown state %s in set_think_progress_cb', state))
               end
            end)
         end

      elseif not should_think and self._thinking then      
         self._thinking = false
         self._execution_frame:stop_thinking()
      end
   end
end

function RunTaskAction:get_debug_info(depth)
   if not self._debug_info then
      self._debug_info = radiant.create_datastore()
      self._debug_info:modify(function(o)
            o.id = self._id
            o.depth = depth + 1
            o.name = self._activity.name
            o.args = stonehearth.ai:format_args(self._activity.args)
            o.does = self.does
            o.priority = self.priority
            if self._execution_frame then
               o.execution_frames = { self._execution_frame:get_debug_info(o.depth) }
            end
         end)
   end
   return self._debug_info
end

function RunTaskAction:start_thinking(ai, entity)
   self._entity = entity
   
   self:_create_execution_frame(ai)

   self._should_think = true
   self:_start_stop_thinking()
end

function RunTaskAction:stop_thinking(ai, entity)
   self._should_think = false
   self:_start_stop_thinking()
end

function RunTaskAction:start(ai)
   self._starting = true
   if not self._task:__action_try_start(self) then
      ai:abort('task would not allow us to start (max running reached?)')
      return
   end
   self._starting = false
   self._execution_frame:start()
end

function RunTaskAction:run(...)
   self._execution_frame:run(...)
   self._task:__action_completed(self)
end

function RunTaskAction:stop()
   if self._execution_frame then
      self._execution_frame:stop()
   end
   self._task:__action_stopped(self)
end

function RunTaskAction:destroy()
   self._task:__action_destroyed(self)
   self._task = nil
   self._activity = nil
   if self._execution_frame then
      self._execution_frame:destroy()
      self._execution_frame = nil
   end
   
   if self._debug_info then
      radiant.destroy_datastore(self._debug_info)
      self._debug_info = nil
   end

   if self._started_listener then
      self._started_listener:destroy()
      self._started_listener = nil
   end

   if self._stopped_listener then
      self._stopped_listener:destroy()
      self._stopped_listener = nil
   end

   if self._work_available_listener then
      self._work_available_listener:destroy()
      self._work_available_listener = nil
   end
end

function RunTaskAction:_frame_not_ready(frame)
   assert(frame)
   if self._execution_frame == frame then
      self._log:debug('current frame became unready')
      self._ai:clear_think_output()
   end
end

return RunTaskAction
