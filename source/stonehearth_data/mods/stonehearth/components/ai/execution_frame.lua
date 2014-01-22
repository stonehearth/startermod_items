local Point3 = _radiant.csg.Point3
local ExecutionFrame = class()

local SET_ACTIVE_UNIT = { __comment = 'change active running unit' }

-- these values are in the interface, since we trigger them as events.  don't change them!
local THINKING = 'thinking'
local CALLING_START_THINKING = 'calling_start_thinking'
local READY = 'ready'
local STARTING = 'starting'
local RUNNING = 'running'
local FINISHED = 'finished'
local STOPPING = 'stopping'
local STOPPED = 'stopped'
local DYING = 'dying'
local DEAD = 'dead'

function ExecutionFrame:__init(parent_thread, entity, name, args, action_index)
   self._id = stonehearth.ai:get_next_object_id()
   self._name = name
   self._args = args
   self._entity = entity
   self._action_index = action_index
   self._parent_thread = parent_thread
   self._execution_units = {}

   self._thread = parent_thread:create_thread()
                               :set_debug_name("f:%d", self._id)
                               :set_msg_handler(function (...) self._dispatch_msg(...) end)
                               :set_thread_main(function (...) self:_thread_main(...) end)
                               :start()

   self._log = radiant.log.create_logger('ai.exec_frame')
   local prefix = string.format('%s (%s)', self._thread:get_debug_route(), self._name)
   self._log:set_prefix(prefix)
   self._log:debug('creating execution frame')
   
   self:_set_state(STOPPED)
end

-- external interface from another thread.  blocking run (only)
function ExecutionFrame:start_thinking(entity_state)
      frame:set_current_entity_state(entity_state)
end

function ExecutionFrame:run()
   self._calling_thread = stonehearth.threads:get_running_thread()

   if self._state == STOPPED then
      self._thread:send_msg('start_thinking')
   end
   while not self._in_state(READY, DEAD) do
      self._calling_thread:suspend()
   end
   self._thread:send_msg('start')
   while not self._in_state(STARTED, DEAD) do
      self._calling_thread:suspend()
   end
   self._thread:send_msg('run')
   while not self._in_state(HALTED, DEAD) do
      self._calling_thread:suspend()
   end
end

function ExecutionFrame:stop()
   assert(not self._calling_thread)
   self._calling_thread = radiant.thread:get_running_thread()

   if self._state == STOPPED then
      return
   end
   self._thread:send_msg('stop')
   while not self._in_state(STOPPED, DEAD) do
      self._calling_thread:suspend()
   end
end

function ExecutionFrame:_create_execution_units()
   local actions = self._action_index[self._name]
   if not actions then
      log:warning('no actions for %s at the moment.  this is actually ok for tasks (they may come later)', self._name)
      return
   end
   for key, entry in pairs(actions) do
      self:_add_execution_unit(key, entry)
   end
end

function ExecutionFrame:_dispatch_msg(msg, ...)
   local method = '_' .. msg .. '_from_' .. self._state
   local fn = self[method]
   assert(fn)
   fn(self, ...)
end


function ExecutionFrame:_thread_main()
   self:_create_execution_units()
   while self._state ~= DEAD do
      self._thread:suspend()
   end
end

function ExecutionFrame:_start_thinking_from_stopped()
   self._log:debug('_start_thinking_from_stopped (%d units)', #self._execution_units)
   assert(self._active_unit == nil)

   self:_set_state(THINKING)
   for _, unit in pairs(self._execution_units) do
      unit:send_msg(START_THINKING, self:_clone_entity_state())
   end
end

function ExecutionFrame:_unit_ready_from_thinking(unit)
   if not self._active_unit then
      self._log:detail('selecting first ready unit "%s"', unit:get_name())
      self._active_unit = unit
      self:send_msg('check_ready')
   end
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
   self:_set_state(HALTED)
end

function ExecutionFrame:_stop_from_halted(unit)
   self:_set_state(STOPPING)
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('stop')
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_add_action_from_anystate(key, entry, does)
   local unit = self:_add_execution_unit(key, entry)

   if self:_in_state(THINKING, READY) then
      unit:start_thinking(self:_clone_entity_state())
   elseif self:_in_state(RUNNING) then
      if self:_is_strictly_better_than_active(unit) then
         self:stop()
         self:capture_entity_state()
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

function ExecutionFrame:destroy()
   assert(false, 'meh...')
   --assert(self._name ~= 'stonehearth:top')
   
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

function ExecutionFrame:set_current_entity_state(state)
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

function ExecutionFrame:capture_entity_state()
   self._log:debug('capture_entity_state')
   assert(self:_in_state(STOPPED))
   
   local state = {
      location = radiant.entities.get_world_grid_location(self._entity),
      carrying = radiant.entities.get_carrying(self._entity),
   }
   self:set_current_entity_state(state)
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
 