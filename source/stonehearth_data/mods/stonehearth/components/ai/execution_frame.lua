local StateMachine = require 'components.ai.state_machine'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_random_number_generator()
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

function ExecutionFrame._add_nop_state_transition(msg, state)
   local method = '_' .. msg .. '_from_' .. state
   assert(not ExecutionFrame[method])
   ExecutionFrame[method] = function() end
end

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
                               :set_user_data(self)
                               :set_thread_main(function (...) self:_thread_main(...) end)

   self._log = radiant.log.create_logger('ai.exec_frame')
   local prefix = string.format('%s (%s)', self._thread:get_debug_route(), self._name)
   self._log:set_prefix(prefix)
   self._log:debug('creating execution frame')
   
   self._sm = StateMachine(self, self._thread, self._log, STOPPED)
   self:_set_state(STOPPED)

   self._thread:start()
end

-- does not block until ready... just starts the thinking
-- process...  you probably don't want this.  you probably
-- want run.
function ExecutionFrame:start_thinking(entity_state, ready_cb)
   self._log:spam('start_thinking')
   assert(entity_state)
   assert(not self._ready_cb)
   assert(self:_get_state() == STOPPED)

   self._ready_cb = ready_cb
   self._thread:send_msg('start_thinking', entity_state)
end

function ExecutionFrame:stop_thinking()
   self._log:spam('stop_thinking')
   self._sm:protect_blocking_call(function(caller)
         self._thread:send_msg('stop_thinking')
         self._sm:wait_until(STOPPED)
      end)
end

function ExecutionFrame:start()
   self._log:spam('start')
   self._sm:protect_blocking_call(function(caller)   
         self._thread:send_msg('start')
         self._sm:wait_until(STARTED)
      end)
end

function ExecutionFrame:run()
   self._log:spam('run')         
   self._sm:protect_blocking_call(function(caller)   
         self._thread:send_msg('start_thinking')
         self._sm:wait_until(READY)

         self._thread:send_msg('start')
         self._sm:wait_until(STARTED)
         
         self._thread:send_msg('run')
         self._sm:wait_until(HALTED)
      end)
end

function ExecutionFrame:stop()
   self._log:spam('stop')
   self._sm:protect_blocking_call(function(caller)   
         self._thread:send_msg('stop')
         self._sm:wait_until(STOPPED)
      end)
end

function ExecutionFrame:shutdown()
   self._log:spam('shutdown')
   self._sm:protect_blocking_call(function(caller)
         self._thread:send_msg('shutdown')
         self._thread:wait()
      end)
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

function ExecutionFrame:_thread_main()
   self:_create_execution_units()
   self._sm:loop_until(DEAD)
end

function ExecutionFrame:_shuffle_execution_units()
   local units = {}
   for _, unit in pairs(self._execution_units) do
      table.insert(units, unit)
   end
   local c = #units
   for i = c, 2, -1 do
      local j = rng:get_int(1, i)
      units[i], units[j] = units[j], units[i]
   end
   return units
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

   -- randomize the order that we tell execution units to start thinking based on the
   -- number of shares they have.   the first execution unit we notify has a better
   -- chance of being the active one.
   local shuffled = self:_shuffle_execution_units()
   for _, unit in ipairs(shuffled) do
      unit:send_msg('start_thinking', self:_clone_entity_state())
   end
end

function ExecutionFrame:_stop_thinking_from_ready(entity_state)
   self._log:debug('_stop_thinking_from_ready')

   assert(self._active_unit)
   self._active_unit = nil
   
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('stop_thinking')
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_unit_ready_from_thinking(unit, think_output)
   if not self._active_unit then
      self._log:detail('selecting first ready unit "%s"', unit:get_name())
   elseif self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s"',
                       self._active_unit:get_name(), unit:get_name())
   else
      return
   end
   self._active_unit = unit
   self._active_unit_think_output = think_output
   self:send_msg('check_ready')
end

function ExecutionFrame:_unit_ready_from_running(unit, think_output)
   assert(self._active_unit)
   if self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s" while running!!',
                       self._active_unit:get_name(), unit:get_name())

      -- kill just this individual unit
      local dead_unit = self._active_unit
      self._active_unit = nil
      dead_unit:send_msg('terminate', string.format('replaced by unit "%s"', unit:get_name()))
      self:_remove_execution_unit(dead_unit)

      -- go back to the ready state and start.
      self._active_unit = unit
      self:_set_state(READY)
      self:send_msg('start')
   end
end

function ExecutionFrame:_unit_not_ready_from_thinking(unit)
   if unit == self._active_unit then
      self._log:detail('best choice for active_unit "%s" became unready...', unit:get_name())
      self:_find_replacement_for_active_unit()
   end
end

function ExecutionFrame:_child_thread_exit_from_thinking(thread, err)
   local unit = thread:get_user_data()
   if unit == self._active_unit then
      self._log:detail('active_unit "%s" reported error...', unit:get_name())
      self:_find_replacement_for_active_unit()
   end
   self:_remove_execution_unit(unit)
end

function ExecutionFrame:_child_thread_exit_from_stopped(thread, err)
   -- nothing to do
end

function ExecutionFrame:_find_replacement_for_active_unit()
   self._active_unit = self:_get_best_execution_unit()
   if self._active_unit then
      self._log:detail('selected "%s" as a replacement...', self._active_unit)
   else
      self._log:detail('could not find a replacement!')
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

function ExecutionFrame:_shutdown_from_stopped()
   self:_shutdown()
end

function ExecutionFrame:_shutdown_from_halted()
   self:_shutdown()
end

-- die a horrible death.  does not return
function ExecutionFrame:_terminate_from_anystate(reason)
   assert(reason)
   
   self:_set_state(DEAD)
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('terminate')
   end
   self:_terminate(reason)
end

function ExecutionFrame:_shutdown()
   self._active_unit = nil
   self:_set_state(DEAD)  
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('shutdown')
   end
   self._execution_units = {}
   self._thread:wait_for_children()
end


-- terminate is private because it's only valid to call it on our thread
-- after we've called stop_thinking, stop, and destroy on the action
function ExecutionFrame:_terminate(reason)
   -- terminate to ensure that we process no more messeages.
   assert(self._thread:is_running())
   self._thread:terminate(reason)
   radiant.not_reached()
end

function ExecutionFrame:_add_action_from_anystate(key, entry, does)
   error('not correct...')
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
   error('not correct...')
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

-- assumes the unit is already destroyed or aborted.  we just need to remove
-- it from our list
function ExecutionFrame:_remove_execution_unit(unit)
   assert(unit ~= self._active_unit)
   for key, u in pairs(self._execution_units) do
      if unit == u then
         self._execution_units[key] = nil
         return
      end
   end
end

function ExecutionFrame:on_action_index_changed(add_remove, key, entry, does)
   if self._name == does then
      self:send_msg(add_remove .. '_action', key, entry)
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
   assert(self:_get_state() == STOPPED)

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

function ExecutionFrame:get_debug_info()
   local info = {
      id = self._id,
      state = self:_get_state(),
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

function ExecutionFrame:_set_state(state)
   return self._sm:set_state(state)
end

function ExecutionFrame:_in_state(state)
   return self._sm:in_state(state)
end

function ExecutionFrame:_get_state(state)
   return self._sm:get_state(state)
end

StateMachine.add_nop_state_transition(ExecutionFrame, 'unit_stopped', STOPPED)
StateMachine.add_nop_state_transition(ExecutionFrame, 'child_thread_exit', DEAD)

return ExecutionFrame
 