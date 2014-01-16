local Point3 = _radiant.csg.Point3
local ExecutionFrame = class()

local SET_ACTIVE_UNIT = { __comment = 'change active running unit' }

-- these values are in the interface, since we trigger them as events.  don't change them!
local CONSTRUCTED = 'constructed'
local THINKING = 'thinking'
local READY = 'ready'
local STARTING = 'starting'
local RUNNING = 'running'
local FINISHED = 'finished'
local STOPPING = 'stopping'
local STOPPED = 'stopped'
local DYING = 'dying'
local DEAD = 'dead'

function ExecutionFrame:__init(ai_component, actions, activity, debug_route)
   self._ai_component = ai_component
   self._entity = self._ai_component:get_entity()
   self._activity = activity
   self._state = CONSTRUCTED
   self._execution_units = {}
   self._id = stonehearth.ai:get_next_object_id()
   self._buffer_changes_depth = 0
   self._modified_state = {
      changed = false,
      units = {},
   }

   self._log = radiant.log.create_logger('ai.exec_frame')
   if not debug_route then
      debug_route = 'e:' .. tostring(self._entity:get_id())
   end
   self:set_debug_route(debug_route)  
   self._log:debug('creating execution frame')
   
   local ai = stonehearth.ai
   for key, entry in pairs(actions) do
      self:_add_execution_unit(key, entry)
   end
   
   -- notify everyone that they party's started
   for _, unit in pairs(self._execution_units) do
      self:_init_execution_unit(unit)
   end
   radiant.events.listen(self._ai_component, 'action_added',   self, self._on_action_added)
   radiant.events.listen(self._ai_component, 'action_removed', self, self._on_action_removed)
end

function ExecutionFrame:_on_action_added(key, entry, does)
   if self._activity.name == does then
      local unit = self:_add_execution_unit(key, entry)
      self:_init_execution_unit(unit)
      if self:_is_thinking() then
         if self:_is_better_execution_unit(unit) then
            unit:start_thinking(self:_clone_entity_state())
         end
      end
   end
end

function ExecutionFrame:_on_action_removed(removed_key, entry, does)
   if self._activity.name == does then
      for key, unit in pairs(self._execution_units) do
         if key == removed_key then
            if unit == self._active_unit then
               self:stop()
            end
            unit:stop()
            unit:destroy()
            self._execution_units[key] = nil
            break
         end
      end
   end
end

function ExecutionFrame:_is_thinking()
   return self._state == THINKING or
          self._state == READY or
          self._state == RUNNING or
          self._state == FINISHED
end

function ExecutionFrame:_add_execution_unit(key, entry)
   local name = tostring(type(key) == 'table' and key.name or key)
   self._log:spam('adding new execution unit: %s', name)
   
   assert(not self._execution_units[key])   
   local unit = stonehearth.ai:create_execution_unit(self._ai_component,
                                                     self._debug_route,
                                                     entry.action_ctor,
                                                     self._entity,
                                                     entry.injecting_entity)
   self._execution_units[key] = unit
   return unit
end

function ExecutionFrame:_init_execution_unit(unit)
   unit:set_debug_route(self._debug_route)
   unit:initialize(self._activity.args)
   radiant.events.listen(unit, 'state_changed', self, self.on_unit_state_change)
end

function ExecutionFrame:set_debug_route(debug_route)
   self._debug_route = debug_route .. ' f:' .. tostring(self._id)
   local prefix = string.format('%s (%s)', self._debug_route, self._activity.name)
   self._log:set_prefix(prefix)
   for _, unit in pairs(self._execution_units) do
      unit:set_debug_route(self._debug_route)
   end   
end

function ExecutionFrame:destroy()
   self._log:debug('destroy')
   if self._state ~= DEAD and self._state ~= DYING then
      assert(not self._co_running, 'destroy of a running execution frame not implemented (yet)')
      local execution_units = self._execution_units
      
      -- mark our state as dead first, to prevent the subsequent cleanup from doing
      -- anything wasteful
      self:_set_state(DYING)
      
      self._co = nil
      self._active_unit = nil
      self._execution_units = {}
      for _, unit in pairs(execution_units) do
         radiant.events.unlisten(unit, 'state_changed', self, self.on_unit_state_change)
         unit:stop()
         unit:destroy()
      end     
      self:_set_state(DEAD)
      radiant.events.unpublish(self)
      radiant.events.unlisten(self._ai_component, 'action_added', self, self._on_action_added)
      radiant.events.unlisten(self._ai_component, 'action_removed', self, self._on_action_removed)
   else
      self._log:spam('ignoring redundant destroy call (this is not an error, just letting you know)')
   end
end

function ExecutionFrame:get_activity()
   return self._activity
end

function ExecutionFrame:get_active_execution_unit()
   return self._active_unit
end

-- xxx
function ExecutionFrame:on_unit_state_change(unit, state)
   self._log:spam('on_unit_state_change %s -> %s (throttle depth: %d)', unit:get_name(), state, self._buffer_changes_depth)
   if self._buffer_changes_depth > 0 then
      self._modified_state.changed = true
      self._modified_state.units[unit] = state
      return
   end
   if self:_in_state(THINKING, READY, RUNNING) then
      if unit:is_runnable() and self:_is_better_execution_unit(unit) then
         self._log:spam('active unit "%s" being replaced by unit "%s"', self:_get_active_unit_name(), unit:get_name())
         if self._state == RUNNING then
            self:_set_active_unit(unit)
         else
            self:_set_active_unit(unit)
            self:_set_state(READY)
         end
      elseif unit == self._active_unit and not self._active_unit:is_active() then
         local new_unit = self:_get_best_execution_unit()
         if new_unit then
            self._log:spam('active unit %s has become unrunable.  replacing with unit %s',
                           self:_get_active_unit_name(), new_unit and new_unit:get_name() or 'nil')
            self:_set_active_unit(new_unit)
         else
            self._log:spam('no more runable units in frame!')
            self:_set_active_unit(nil)
         end
      end
   end
end

function ExecutionFrame:start_thinking()
   self._log:spam('start_thinking (current unit:%s)', self:_get_active_unit_name())
   assert(not self._co)
   assert(not self._co_running)
   assert(self._state == CONSTRUCTED)

   self:_set_state(THINKING)
   self:_buffer_state_changes(function()   
         for _, unit in pairs(self._execution_units) do
            if self:_is_better_execution_unit(unit) then
               unit:start_thinking(self:_clone_entity_state())
            end
         end
      end)
end

function ExecutionFrame:stop_thinking()
   self._log:spam('stop_thinking (state:%s)', self._state)
   
   self:_buffer_state_changes(function()   
         if self._state == CONSTRUCTED  then
            assert(self._active_unit == nil)
         elseif self:_in_state(THINKING, READY, STARTING, RUNNING, FINISHED, STOPPING, STOPPED) then
            for _, unit in pairs(self._execution_units) do
               unit:stop_thinking()
            end
         elseif self._state == DEAD then
         else
            assert(false, string.format('unknown state %s in stop_thinking', self._state))
         end
      end)
end

function ExecutionFrame:start()
   self._log:spam('start')
   assert(not self._co)
   assert(not self._co_running)
   assert(self._state == READY)
   assert(self._active_unit)

   self:_buffer_state_changes(function()   
         self:_set_state(STARTING)
         self._current_entity_state = nil
         for _, unit in pairs(self._execution_units) do
            if unit == self._active_unit then
               unit:start()
               unit:stop_thinking()
            elseif not self:_is_better_execution_unit(unit) then
               -- this guy has no prayer...  just stop
               unit:stop_thinking()
            else
               -- let better ones keep processing.  they may prempt in the future
               -- unit:start_thinking(self:_clone_entity_state())
            end
         end
         
         assert(not self._co)
         self._log:spam('creating new thread for %s', self._active_unit:get_name())
         self._co = coroutine.create(function()
               self._active_unit:run()
            end)   
         self:_set_state(RUNNING)
      end)
end

function ExecutionFrame:set_current_entity_state(state)
   self._log:spam('set_current_entity_state')
   assert(state)
   assert(self._state == CONSTRUCTED)
   for key, value in pairs(state) do
      self._log:spam('  CURRENT.%s = %s', key, tostring(value))
   end
   
   -- we explicitly do not clone this state.  compound actions expect us
   -- to propogate side-effects of executing this frame back in the state
   -- parameter.
   self._current_entity_state = state
   self._saved_entity_state = self:_clone_entity_state()   
end

function ExecutionFrame:_clone_entity_state()
   assert(self._current_entity_state)
   local s = self._current_entity_state
   local cloned = {
      location = Point3(s.location.x, s.location.y, s.location.z),
      carrying = s.carrying,
   }
  self._log:spam('cloning current state %s to %s', tostring(self._current_entity_state), tostring(cloned)) 
  return cloned
end

function ExecutionFrame:run()
   self._log:spam('begin run')
   
   if self._state == CONSTRUCTED then
      self:start_thinking()
   end
   repeat
      if self._state == THINKING then
         self:_suspend_until_ready()
      elseif self._state == READY then
         self:start()
      elseif self._state == RUNNING then
         assert(self._co)
         self:_protected_call()
      else
         assert(false, string.format('unknown state %s in state machine', self._state))
      end
   until self._state == FINISHED or self._state == DEAD   
   self._log:spam('end run (state is %s)', self._state)
end

function ExecutionFrame:capture_entity_state()
   self._log:spam('capture_entity_state')
   assert(self:_in_state(CONSTRUCTED, STOPPED))
   
   local state = {
      location = radiant.entities.get_world_grid_location(self._entity),
      carrying = radiant.entities.get_carrying(self._entity),
   }
   self:set_current_entity_state(state)
end

function ExecutionFrame:loop()
   while true do
      self:capture_entity_state()
      self:run()
      self._active_unit:stop()
      self:_set_active_unit(nil)
      
      -- at this point, there may still be tasks hanging around still thinking
      -- because they were higher priority than the unit that just completed.
      -- that's ok, because we're about to spin around and 'start_thinking' again.
      -- the execution unit is smart enough to stop/start in this case.
      --self._active_unit:start_thinking(self:_clone_entity_state())
      
      self:_set_state(CONSTRUCTED)
   end
end

function ExecutionFrame:stop()
   self._log:spam('stop')

   self:_buffer_state_changes(function()      
         if self._state ~= STOPPED then
            self:_set_active_unit(nil)      
            self:_set_state(STOPPING)
            for _, unit in pairs(self._execution_units) do
               unit:stop()
            end
            self:_set_state(STOPPED)
         end
      end)
end

function ExecutionFrame:_suspend_until_ready()
   assert(not self._co)
   assert(not self._co_running)
   assert(not self._active_unit)
   assert(self._state == THINKING)

   self._log:info('suspending thread until ready to go')
   radiant.events.listen(self, 'ready', function()
      self._ai_component:resume_thread()
      return radiant.events.UNLISTEN
   end)
   self._ai_component:suspend_thread()
   self._log:info('ready! thread resumed')
   
   assert(self._active_unit)
   assert(self._active_unit:is_runnable() and self._active_unit:get_priority() > 0)
   assert(self._state == READY)
end

-- *strictly* better.  no run-offs for latecomers.
function ExecutionFrame:_is_better_execution_unit(unit)
   if not self._active_unit then
      return true
   end
   if unit == self._active_unit then
      return false
   end
   if unit:get_priority() > self._active_unit:get_priority() then
      return true
   end
   return false
end

function ExecutionFrame:_get_best_execution_unit()
   local best_priority = 0
   local active_units = nil
   
   self._log:spam('%s looking for best execution unit for "%s"', self._entity, self._activity.name)
   for _, unit in pairs(self._execution_units) do
      local name = unit:get_name()
      local priority = unit:get_priority()
      local is_runnable = unit:is_runnable()
      local is_active = unit:is_active()
      self._log:spam('  unit %s has priority %d (runnable: %s active: %s)', name, priority, tostring(is_runnable), tostring(is_active))
      if is_runnable or is_active then
         if priority >= best_priority then
            if priority > best_priority then
               -- new best_priority found, wipe out the old list of candidate actions
               active_units = {}
            end
            best_priority = priority
            for i = 1, unit:get_weight() do
               table.insert(active_units, unit)
            end
         end
      end
   end

   if not active_units then
      self._log:debug('no unit ready to run yet.  returning nil.')
      return nil
   end
   
   -- choose a random unit amoung all the units with the highest priority (they all tie)
   local active_unit = active_units[math.random(#active_units)]
   self._log:spam('%s  best unit for "%s" is "%s" (priority: %d)', self._entity, self._activity.name, active_unit:get_name(), best_priority)
   
   return active_unit
end

function ExecutionFrame:_get_active_unit_name()
   return self._active_unit and self._active_unit:get_name() or 'nil'
end

function ExecutionFrame:_set_active_unit(unit)
   assert(not self._co_running)
   
   self._log:spam('changing active unit "%s" -> "%s" in %s state', self:_get_active_unit_name(),
                  unit and unit:get_name() or 'nil', self._state)
   if unit then
      assert(unit:is_runnable() and unit:get_priority() > 0)
   end

   local set_active_unit = function(u)
      self:_spam_current_state('before switching active units')
      self._active_unit = u
      self._log:spam('switched execution unit to "%s"', self:_get_active_unit_name())
      -- don't kill the table.  it's a shared reference
      local new_entity_state
      if u then
         new_entity_state = u:get_current_entity_state()
      else
         new_entity_state = self._saved_entity_state
      end
      if self._current_entity_state then
         self._log:spam('copying unit state %s to current state %s', tostring(new_entity_state), tostring(self._current_entity_state)) 
         for name, value in pairs(new_entity_state) do
            self._current_entity_state[name] = value
         end
      end
      self:_spam_current_state('after switching active units')
   end
   
   if self._state == CONSTRUCTED then
      assert(not unit)
      assert(not self._active_unit)
      assert(not self._co)
   elseif self._state == THINKING then
      assert(unit)
      assert(not self._active_unit)
      assert(not self._co)
      set_active_unit(unit)
   elseif self._state == READY then
      assert(not self._co)
      set_active_unit(unit)
   elseif self._state == RUNNING then
      assert(self._co)
      assert(not self._co_running)
      assert(self._active_unit)
      
      self:_buffer_state_changes(function()
            if unit then
               local outgoing_unit = self._active_unit
               local msg = string.format('terminating %s so better action %s can go.', outgoing_unit:get_name(), unit:get_name())
               self._log:spam(msg)
               
               self._co = nil
               self._co_running = false
               set_active_unit(unit)


               outgoing_unit:stop()
               set_active_unit(unit)
               -- would it be better to go into processing from here and call start_thinking on the unit?
               -- almost certainly yes.  who are we to judge that it doesn't want to do something differently?
               self:_set_state(READY)
            else
               self._log:spam('terminating %s while running (by request)', self._active_unit:get_name())
               self._co = nil
               self._co_running = false
               set_active_unit(nil)
            end
            self._ai_component:resume_thread()
         end)
   elseif self._state == FINISHED then
      assert(not unit)
      assert(not self._co)
      assert(not self._co_running)
      set_active_unit(nil)
   elseif self._state == STOPPED then
      assert(not self._co)
      assert(not self._active_unit)
      assert(not unit)
   else
      -- not yet implemented or error
      assert(false, string.format('invalid state %s in set_active_unit', self._state))
   end
end

function ExecutionFrame:_in_state(...)
   for _, state in ipairs({...}) do
      if self._state == state then
         return true
      end
   end
   return false
end

function ExecutionFrame:_set_state(state)
   local current_state = self._modified_state.state or self._state
   self._log:spam('state change %s -> %s (throttle depth: %d)', tostring(current_state), state, self._buffer_changes_depth)

   --[[
   if self._buffer_changes_depth > 0 then
      self._modified_state.changed = true
      self._modified_state.state = state
      return
   end
   ]]
   self._state = state
   radiant.events.trigger(self, self._state, self)
   radiant.events.trigger(self, 'state_changed', self, self._state)
end

function ExecutionFrame:_protected_call()
   assert(self._co)
   assert(not self._co_running)
   assert(self._active_unit)
   
   -- resume the co-routine...
   local current_unit = self._active_unit
   self._log:spam('making protected pcall for "%s"', current_unit:get_name())
   self._co_running = true
   local success, result = coroutine.resume(self._co)
   self._co_running = false
   self._log:spam('protected pcall for "%s" returned (active unit is now "%s")', 
                  current_unit:get_name(), self:_get_active_unit_name())
   
   assert(self._co)
   if not success then
      radiant.check.report_thread_error(self._co, 'entity ai error: ' .. tostring(result))
      self:destroy()
      return
   end

   -- check the status...
   local status = coroutine.status(self._co)
   if status == 'dead' then
      -- the coroutine's status is 'dead' when it runs off the end of its main.
      -- this isn't an error, but we're done with the current action.
      self._log:spam('coroutine is finished. returning from protected call')
      self._co = nil
      -- we explicity do not call stop here.  make the caller do it, so they have
      -- explicit control of the destruction of many spawned execution frames.
      -- use the state FINISHED to indicate run is finished but stop has not been
      -- called
      self:_set_state(FINISHED)
   elseif status == 'suspended' then
      -- the coroutine's status is suspended when the run method yields.  this
      -- almost always happens because the run method called ai:suspend(). 
      -- continue unwinding
      self._log:spam('coroutine called thread_suspend.  suspending...')
      assert(result)
      coroutine.yield(result)
      -- annnnd, we're back!  nearly anything and everything may have changed.
      -- take extra special care to validate our current state before proceeding
      -- further
      self._log:spam('resuming from thread_suspend (current unit is now "%s").', self:_get_active_unit_name())
   else  
      assert(false, 'coroutine in unknown status %s', status)
   end         
end

function ExecutionFrame:get_debug_info()
   local info = {
      id = self._id,
      state = self._state,
      activity = {
         name = self._activity.name,
      },
      args = stonehearth.ai:format_args(self._activity.args),
      execution_units = {}
   }
   for _, unit in pairs(self._execution_units) do
      table.insert(info.execution_units, unit:get_debug_info())
   end
   return info
end

function ExecutionFrame:_spam_current_state(msg)
   self._log:spam(msg)
   if self._current_entity_state then
      self._log:spam('  CURRENT is %s', tostring(self._current_entity_state))
      for key, value in pairs(self._current_entity_state) do      
         self._log:spam('  CURRENT.%s = %s', key, tostring(value))
      end   
   else
      self._log:spam('  no CURRENT state!')
   end
end

function ExecutionFrame:get_state()
   return self._state
end

function ExecutionFrame:_push_modified_state()
   if self._buffer_changes_depth == 0 then
      self._log:spam('restoring state after throttle')
      while self._modified_state.changed do 
         self._modified_state.changed = false
         if self._modified_state.state and self._modified_state.state ~= self._state then
            local new_state = self._modified_state.state
            self._modified_state.state = nil
            self:_set_state(new_state)
         end
         for modified_unit, modified_state in pairs(self._modified_state.units) do
            self._modified_state.units[modified_unit] = nil
            for k, v in pairs(self._execution_units) do
               if v == modified_unit then
                  self:on_unit_state_change(modified_unit, modified_state)
               end
            end
         end
      end
      self._log:spam('finished restoring state')
   end
end

function ExecutionFrame:_buffer_state_changes(fn)
   self._buffer_changes_depth = self._buffer_changes_depth + 1
   self._log:spam('throttling all state changes (depth is now %d)', self._buffer_changes_depth)
   fn()
   self._log:spam('finished throttling all state changes (depth is now %d)', self._buffer_changes_depth)
   self._buffer_changes_depth = self._buffer_changes_depth - 1

   self:_push_modified_state()
end

return ExecutionFrame
 