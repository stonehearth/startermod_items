local Point3 = _radiant.csg.Point3
local ExecutionFrame = class()

local SET_ACTIVE_UNIT = { __comment = 'change active running unit' }

-- these values are in the interface, since we trigger them as events.  don't change them!
local THINKING = 'thinking'
local READY = 'ready'
local STARTING = 'starting'
local STARTED = 'started'
local RUNNING = 'running'
local HALTED = 'halted'
local STOPPING = 'stopping'
local STOPPED = 'stopped'
local DYING = 'dying'
local DEAD = 'dead'

function ExecutionFrame:__init(parent_thread, entity, name, args, action_index)
   self._id = stonehearth.ai:get_next_object_id()
   self._name = name
   self._args = args or {}
   self._entity = entity
   self._action_index = action_index
   self._parent_thread = parent_thread
   self._execution_units = {}

   self._thread = parent_thread:create_thread()
                               :set_debug_name("f:%d", self._id)
                               :set_msg_handler(function (...) self:_dispatch_msg(...) end)
                               :set_thread_main(function (...) self:_thread_main(...) end)

   self._log = radiant.log.create_logger('ai.exec_frame')
   local prefix = string.format('%s (%s)', self._thread:get_debug_route(), self._name)
   self._log:set_prefix(prefix)
   self._log:debug('creating execution frame')
   
   self:_set_state(STOPPED)
   self._thread:start()
end

-- external interface from another thread.  blocking.
function ExecutionFrame:_protect_blocking_call(blocking_call_fn)
   assert(not self._calling_thread)

   self._calling_thread = stonehearth.threads:get_running_thread()   
   assert(self._calling_thread ~= self._thread)
   local result = { blocking_call_fn(self._calling_thread) }
   self._calling_thread = nil

   return unpack(result)
end

function ExecutionFrame:start_thinking(entity_state)
   self._log:spam('start_thinking')
   assert(entity_state)
   
   return self:_protect_blocking_call(function(caller)   
         self._thread:send_msg('start_thinking', entity_state)
         self:_block_caller_until(READY)
         
         assert(self._active_unit_think_output)
         return self._active_unit_think_output
      end)
end

function ExecutionFrame:run()
   self._log:spam('run')         
   self:_protect_blocking_call(function(caller)   
         self._thread:send_msg('start_thinking')
         self:_block_caller_until(READY)

         self._thread:send_msg('start')
         self:_block_caller_until(STARTED)
         
         self._thread:send_msg('run')
         self:_block_caller_until(HALTED)
      end)
end

function ExecutionFrame:stop()
   self._log:spam('stop')
   self:_protect_blocking_call(function(caller)
         self._thread:send_msg('stop')
         self:_block_caller_until(STOPPED)
      end)
end

function ExecutionFrame:destroy()
   self._log:spam('destroy')
   self:_protect_blocking_call(function(caller)
         self._thread:send_msg('destroy')
         self:_block_caller_until(DEAD)
      end)
   --assert(self._name ~= 'stonehearth:top')
end

function ExecutionFrame:send_msg(...)
   self._thread:send_msg(...)
end

function ExecutionFrame:_create_execution_units()
   local actions = self._action_index[self._name]
   if not actions then
      self._log:warning('no actions for %s at the moment.  this is actually ok for tasks (they may come later)', self._name)
      return
   end
   for key, entry in pairs(actions) do
      self:_add_execution_unit(key, entry)
   end
end

function ExecutionFrame:_dispatch_msg(msg, ...)
   local method = '_' .. msg .. '_from_' .. self._state
   local fn = self[method]
   assert(fn, string.format('unknown state transition in execution frame "%s"', method))
   self._log:spam('dispatching %s', method)
   fn(self, ...)
end


function ExecutionFrame:_thread_main()
   self._log:spam('starting thread main')
   self:_create_execution_units()
   while self._state ~= DEAD do
      self._thread:suspend('ex_frame is idle')
   end
   self._log:spam('exiting thread main')
end

function ExecutionFrame:_start_thinking_from_stopped(entity_state)
   self._log:debug('_start_thinking_from_stopped')
   if entity_state then
      self:_set_current_entity_state(entity_state)
   else
      self:_capture_entity_state()
   end
   assert(self._active_unit == nil)
   assert(self._current_entity_state)

   self:_set_state(THINKING)
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('start_thinking', self:_clone_entity_state())
   end
end

function ExecutionFrame:_unit_ready_from_thinking(unit, think_output)
   if not self._active_unit then
      self._log:detail('selecting first ready unit "%s"', unit:get_name())
      self._active_unit = unit
      self._active_unit_think_output = think_output
   elseif self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s"',
                       self._active_unit:get_name(), unit:get_name())
      self._active_unit = unit
      self._active_unit_think_output = think_output
   end
   self:send_msg('check_ready')
end

function ExecutionFrame:_unit_not_ready_from_thinking(unit)
   if unit == self._active_unit then
      self._log:detail('best choice for active_unit "%s" became unready...', unit:get_name())
      self._active_unit = self:_get_best_execution_unit()
      if self._active_unit then
         self._log:detail('selected "%s" as a replacement...', self._active_unit)
      else
         self._log:detail('could not find a replacement!')
      end
   end
end

function ExecutionFrame:_check_ready_from_thinking()
   if self._active_unit then
      self._log:detail('ready to run!  moving to ready state')
      self:_set_state(READY)
   else
      self._log:detail('not ready to run. keep thinking...')
   end
end

function ExecutionFrame:_check_stopped_from_halted()
   for _, unit in pairs(self._execution_units) do
      if not unit:in_state(STOPPED) then
         return
      end
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_start_from_ready()
   assert(self._active_unit)

   self._current_entity_state = nil
   for _, unit in pairs(self._execution_units) do
      if unit == self._active_unit then
         unit:send_msg('start')
      elseif not self:_is_strictly_better_than_active(unit) then
         unit:send_msg('stop_thinking') -- this guy has no prayer...  just stop
      else
         -- let better ones keep processing.  they may prempt in the future
         -- xxx: there's a problem... once we start running the CURRENT state
         -- will quickly become out of date.  do we stop_thinking/start_thinking
         -- them periodically?
      end
   end
   self:_set_state(STARTED)
end

function ExecutionFrame:_run_from_started()
   assert(self._active_unit)

   self._active_unit:send_msg('run')
   self:_set_state(RUNNING)
end

function ExecutionFrame:_unit_halted_from_running(unit)
   assert(self._active_unit)
   assert(unit == self._active_unit)
   self._active_unit = nil
   self:_set_state(HALTED)
end

function ExecutionFrame:_stop_from_halted()
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('stop')
   end
   self:_check_stopped_from_halted()
end

function ExecutionFrame:_unit_stopped_from_halted(unit)
   self:_check_stopped_from_halted()
end

function ExecutionFrame:_destroy_from_stopped()
   self._active_unit = nil
   error('to be continued, in the morning...')
   --[[
   HMM  NOW WHAT?
   for _, unit in pairs(execution_units) do
      unit:send_msg('destroy')
      
      --- H
      radiant.events.unlisten(unit, 'state_changed', self, self.on_unit_state_change)
      unit:stop()
      unit:destroy()
   end     
   self._execution_units = {}
   
      self:_set_state(DEAD)
      radiant.events.unpublish(self)
      radiant.events.unlisten(self._ai_component, 'action_added', self, self._on_action_added)
      radiant.events.unlisten(self._ai_component, 'action_removed', self, self._on_action_removed)
   else
      self._log:spam('ignoring redundant destroy call (this is not an error, just letting you know)')
   end
   ]]
end

function ExecutionFrame:_add_action_from_anystate(key, entry, does)
   local unit = self:_add_execution_unit(key, entry)

   if self:_in_state(THINKING, READY) then
      unit:start_thinking(self:_clone_entity_state())
   elseif self:_in_state(RUNNING) then
      if self:_is_strictly_better_than_active(unit) then
         self:stop()
         self:_capture_entity_state()
         self:start_thinking()
      end
   end
end

function ExecutionFrame:_remove_action_from_anystate(removed_key, entry)
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

function ExecutionFrame:_on_action_added(key, entry, does)
   if self._name == does then
      self:send_msg('add_action', key, entry)
   end
end

function ExecutionFrame:_on_action_removed(key, entry, does)
   if self._name == does then
      self:send_msg('remove_action', key, entry)
   end
end


function ExecutionFrame:_add_execution_unit(key, entry)
   local name = tostring(type(key) == 'table' and key.name or key)
   self._log:debug('adding new execution unit: %s', name)
   
   assert(not self._execution_units[key])   
   local unit = stonehearth.ai:create_execution_unit(self._thread,
                                                     self._entity,
                                                     entry.injecting_entity,
                                                     entry.action_ctor,
                                                     self._args,
                                                     self._action_index)
   self._execution_units[key] = unit
   return unit
end

function ExecutionFrame:_set_current_entity_state(state)
   self._log:debug('set_current_entity_state')
   assert(state)
   assert(self._state == STOPPED)

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

function ExecutionFrame:_capture_entity_state()
   self._log:debug('_capture_entity_state')
   assert(self:_in_state(STOPPED))
   
   local state = {
      location = radiant.entities.get_world_grid_location(self._entity),
      carrying = radiant.entities.get_carrying(self._entity),
   }
   self:_set_current_entity_state(state)
end

-- *strictly* better.  no run-offs for latecomers.
function ExecutionFrame:_is_strictly_better_than_active(unit)
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
   
   self._log:spam('%s looking for best execution unit for "%s"', self._entity, self._name)
   for _, unit in pairs(self._execution_units) do
      local name = unit:get_name()
      local priority = unit:get_priority()
      local is_runnable = unit:is_runnable()
      self._log:spam('  unit %s has priority %d (runnable: %s)', name, priority, tostring(is_runnable))
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
   self._log:spam('%s  best unit for "%s" is "%s" (priority: %d)', self._entity, self._name, active_unit:get_name(), best_priority)
   
   return active_unit
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
   assert(self._state ~= DEAD)
   self._log:debug('state change %s -> %s', tostring(self._state), state)
   self._state = state
   
   if self._state == self._caller_waiting_for_state then
      self._calling_thread:resume('transitioned to ' .. self._state)
   end
end

function ExecutionFrame:_block_caller_until(state)
   assert(self._calling_thread)
   assert(self._calling_thread:is_running())
   
   if self._state ~= state then
      self._caller_waiting_for_state = state
      while self._state ~= state do
         self._calling_thread:suspend('waiting for ' .. state)
      end
      self._caller_waiting_for_state = nil
   end
end

function ExecutionFrame:get_debug_info()
   local info = {
      id = self._id,
      state = self._state,
      activity = {
         name = self._name,
      },
      args = stonehearth.ai:format_args(self._args),
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

return ExecutionFrame
 