local StateMachine = require 'components.ai.state_machine'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
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
local FINISHED = 'finished'
local STOPPING = 'stopping'
local SWITCHING = 'switching'
local STOPPED = 'stopped'
local UNWINDING = 'unwinding'
local DEAD = 'dead'

local UNWIND_NEXT_FRAME = ':unwind_next_frame:'
local UNWIND_NEXT_FRAME_2 = ':unwind_next_frame_2:'

function ExecutionFrame:__init(thread, debug_route, entity, name, args, action_index)
   self._id = stonehearth.ai:get_next_object_id()
   self._debug_route = debug_route .. ' f:' .. tostring(self._id)
   self._name = name
   self._args = args or {}
   self._entity = entity
   self._action_index = action_index
   self._thread = thread
   self._execution_units = {}
   self._execution_unit_keys = {}
   self._saved_think_output = {}

   self._log = radiant.log.create_logger('ai.exec_frame')
   local prefix = string.format('%s (%s)', self._debug_route, self._name)
   self._log:set_prefix(prefix)
   self._log:debug('creating execution frame')
   
   self._sm = StateMachine(self, self._log, STOPPED)
   self:_set_state(STOPPED)

   self:_create_execution_units()
   self._ai_component = entity:get_component('stonehearth:ai')
   radiant.events.listen(self._ai_component, 'stonehearth:action_index_changed', self, self._on_action_index_changed)
end

function ExecutionFrame:_destroy()
   radiant.events.unlisten(self._ai_component, 'stonehearth:action_index_changed', self, self._on_action_index_changed)
end

function ExecutionFrame:send_msg(...)
   self._sm:dispatch_msg(...)
end

function ExecutionFrame:start_thinking(entity_state)
   self._log:spam('start_thinking')
   assert(entity_state)
   assert(self:get_state() == STOPPED)

   self:send_msg('start_thinking', entity_state)
   if self:_in_state(READY) then
      return self._active_unit_think_output
   end
end

function ExecutionFrame:on_ready(ready_cb)
   self._log:spam('on_ready')
   assert(not self._ready_cb)

   local current_thread = stonehearth.threads:get_current_thread()
   self._ready_cb = function(...)
         local args = { ... }
         if current_thread then
            current_thread:interrupt(function()
                  ready_cb(unpack(args))
               end)
         else
            -- sometimes there is no thread (e.g. in pathfinder callbacks).
            -- just use whatever thread we're on.
            ready_cb(unpack(args))
         end
      end
end

function ExecutionFrame:stop_thinking()
   assert(self._thread:is_running())

   self._log:spam('stop_thinking')
   self:_protected_call(function()
         self:send_msg('stop_thinking')
      end)
end

function ExecutionFrame:start()
   assert(self._thread:is_running())

   self._log:spam('start')
   self:_protected_call(function()
         self:send_msg('start')
         self._sm:wait_until(STARTED)
      end)
end

function ExecutionFrame:run()
   assert(self._thread:is_running())

   self._log:spam('run')

   local run_stack = self._thread:get_thread_data('stonehearth:run_stack')
   table.insert(run_stack, self)

   self:_protected_call(function()
         if not self:_in_state(STOPPED, READY, STARTED) then
            error(string.format('invalid initial state "%s" in run', self:get_state()))
         end

         if self:_in_state(STOPPED) then
            self:send_msg('start_thinking')
            self._sm:wait_until(READY)
         end
         if self:_in_state(READY) then
            self:send_msg('start')
            self._sm:wait_until(STARTED)
         end
         assert(self:_in_state(STARTED))
         
         self:send_msg('run')
         self._sm:wait_until(FINISHED)
      end)

   local last_frame = table.remove(run_stack)

   assert(run_stack == self._thread:get_thread_data('stonehearth:run_stack'))
   assert(self == last_frame)   
end

function ExecutionFrame:stop()
   assert(self._thread:is_running())

   self._log:spam('stop')
   self:_protected_call(function()   
         self:send_msg('stop')
         self._sm:wait_until(STOPPED)
      end)
end

function ExecutionFrame:destroy()
   assert(self._thread:is_running())

   self._log:spam('destroy')
   self:_protected_call(function()   
         self:send_msg('destroy')
         self._sm:wait_until(DEAD)
      end)
end

function ExecutionFrame:send_msg(...)
   self._sm:dispatch_msg(...)
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
      unit:send_msg('start_thinking', self:_clone_entity_state('new speculation for unit'))
   end
end

function ExecutionFrame:_stop_thinking_from_thinking()
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('stop_thinking')
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_thinking_from_ready()
   self._log:debug('_stop_thinking_from_ready')

   assert(self._active_unit)
   self:_set_active_unit(nil)
   
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('stop_thinking')
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_thinking_from_started()
   self._log:debug('_stop_thinking_from_started')
   assert(self._active_unit)
   
   for _, unit in pairs(self._execution_units) do
      if not self:_is_strictly_better_than_active(unit) then
         unit:send_msg('stop_thinking') -- this guy has no prayer...  just stop
      else
         -- let better ones keep processing.  they may prempt in the future
         -- xxx: there's a problem... once we start running the CURRENT state
         -- will quickly become out of date.  do we stop_thinking/start_thinking
         -- them periodically?
      end
   end
end

function ExecutionFrame:_unit_ready_from_thinking(unit, think_output)
   self._saved_think_output[unit] = think_output

   if not self._active_unit then
      self._log:detail('selecting first ready unit "%s"', unit:get_name())
   elseif self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s"',
                       self._active_unit:get_name(), unit:get_name())
   else
      return
   end
   self:_set_active_unit(unit, think_output)
   self:_set_state(READY)
end

function ExecutionFrame:_unit_ready_from_ready(unit, think_output)
   assert(self._active_unit)
   self._saved_think_output[unit] = think_output

   if self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s"',
                       self._active_unit:get_name(), unit:get_name())
   else
      return
   end
   self:_set_active_unit(unit, think_output)
end

function ExecutionFrame:_unit_ready_from_running(unit, think_output)
   assert(self._active_unit)

   self._saved_think_output[unit] = think_output
   
   if self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s" while running!!',
                       self._active_unit:get_name(), unit:get_name())

      -- how do we switch to a different unit?  follow the bouncing ball --> switch_step(...).

      -- switch_step(1) - change our state to SWITCHING so we'll stop dispatching
      -- messages to our regular states (particuarly get us out of the running state!)
      self:_set_state(SWITCHING)
      assert(not self._switch_to_unit)
      self._switch_to_unit = unit
      
      -- switch_step(2) - put an interrupt handler on the thread for this frame (which,
      -- remember, is *not* currently executing) to rewind the stack all the way back
      -- to the current frame
      assert(not self._thread:get_thread_data('stonehearth:unwind_to_frame'))
      self._thread:set_thread_data('stonehearth:unwind_to_frame', self)
      self._thread:interrupt(function()
            assert(self._thread:get_thread_data('stonehearth:unwind_to_frame'))
            
            -- make sure we unwind the frame at the top of the stack.  'self' is the
            -- frame that wants to get ready, which is way up the stack.
            self:_exit_protected_call(UNWIND_NEXT_FRAME)
         end)

   else
      -- not better than our current unit?  just ignore him
   end
end

function ExecutionFrame:_unit_not_ready_from_thinking(unit)
   if unit == self._active_unit then
      self._log:detail('best choice for active_unit "%s" became unready...', unit:get_name())
      self:_find_replacement_for_active_unit()
   end
end

function ExecutionFrame:_set_active_unit(unit, think_output)
   self._active_unit = unit
   self._active_unit_think_output = think_output

   if self._current_entity_state then
      local new_entity_state
      if unit then
         new_entity_state = unit:get_current_entity_state()
         self._log:spam('copying unit state %s to current state %s', tostring(new_entity_state), tostring(self._current_entity_state))
      else
         new_entity_state = self._saved_entity_state
         self._log:spam('copying saved state %s to current state %s', tostring(new_entity_state), tostring(self._current_entity_state))
      end
      for name, value in pairs(new_entity_state) do
         self._current_entity_state[name] = value
      end
   end
end

function ExecutionFrame:_find_replacement_for_active_unit()
   local unit = self:_get_best_execution_unit()
   if self._active_unit then
      local think_output = self._saved_think_output[unit]
      self:_set_active_unit(unit, think_output)
      self._log:detail('selected "%s" as a replacement...', self._active_unit)
   else
      self._log:detail('could not find a replacement!')
      self:_set_active_unit(nil)
   end
end

function ExecutionFrame:_start_from_ready()
   assert(self._active_unit)

   self._current_entity_state = nil
   self:_set_state(STARTING)
   self._active_unit:send_msg('start')
   self:_set_state(STARTED)
end

function ExecutionFrame:_run_from_started()
   assert(self._active_unit)

   self:_set_state(RUNNING)

   local finished, running_unit
   repeat
      running_unit = self._active_unit
      self:_protected_call(function()
            running_unit:send_msg('run')
         end)
         
      assert(self._active_unit)
      if running_unit == self._active_unit then
         finished = true
      else
         self._log:debug('active unit switched from "%s" to "%s" in run.  re-running',
                         running_unit:get_name(), self._active_unit:get_name())
         running_unit = self._active_unit
      end
   until finished

   self._active_unit = nil
   self:_set_state(FINISHED)
end

function ExecutionFrame:_stop_from_finished()
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('stop')
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_destroy_from_stopped()
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('destroy')
   end
   self._execution_units = {}
   self._execution_unit_keys = {}
   self:_set_state(DEAD)
   self:_destroy()
end

function ExecutionFrame:_destroy_from_unwinding()
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:send_msg('destroy')
   end
   self._execution_units = {}
   self._execution_unit_keys = {}
   self:_set_state(DEAD)
   self:_destroy()
end

function ExecutionFrame:_get_top_of_stack()
   local runstack = self._thread:get_thread_data('stonehearth:run_stack')
   return runstack[#runstack]
end

function ExecutionFrame:_unwind_call_stack()
   self._log:debug('_unwind_call_stack')
   local runstack = self._thread:get_thread_data('stonehearth:run_stack')
   for i = #runstack,1,-1 do
      local f = runstack[i]
      self._log:detail('  [%d] - (%5d) %s', i, f._id, f._name, stonehearth.ai.format_args(f._args))
   end
   
   local going_to_frame = self._thread:get_thread_data('stonehearth:unwind_to_frame')
   local current_run_frame = self:_get_top_of_stack()

   assert(going_to_frame)
   assert(current_run_frame)
   assert(current_run_frame == self)
   assert(self._thread:is_running())   

   -- switch_step(3) - we're back the thread that needs to switch to a new
   -- execution unit.  kill the execution unit here.
   assert(self._active_unit)

   -- hold onto this just in case we need it later, then kill the unit
   local current_active_key = self._execution_unit_keys[self._active_unit]
   local unit = self._active_unit
   self._active_unit = nil
   unit:send_msg('stop', true)
   unit:send_msg('destroy')  
   self:_remove_execution_unit(unit)

   -- switch_step(4) - unfortunately, we may be really really deep in nested calls
   -- which all need to be unwound before we can switch.  unwind them all first.
   if current_run_frame ~= going_to_frame then
      -- if we're still stuck down here, we'd best be running.  otherwise why is
      -- our thread suspended?
      assert(self:_in_state(RUNNING))
      self:_set_state(UNWINDING)
      self:send_msg('destroy')
      assert(self:_in_state(DEAD))
      
      -- remove our frame from the runstack, since the guy waiting for the
      -- run to finish won't have the opportunity to
      local run_stack = self._thread:get_thread_data('stonehearth:run_stack')
      table.remove(run_stack)
      
      -- if the next frame is where we want to go, only unwind 1 frame so we
      -- end up back in the protected run loop in :_run_from_*.  otherwise,
      -- jump back 2 frames
      local top = self:_get_top_of_stack()
      if top ~= going_to_frame then
         self:_exit_protected_call(UNWIND_NEXT_FRAME)
      else
         self:_exit_protected_call(UNWIND_NEXT_FRAME_2)
      end
      radiant.not_reached()      
   end

   -- switch_step(7) - we have finally unwound all our called frames (destroying them
   -- along the way), and have reached the switching frame!  before doing so, make a
   -- new unit to replace the guy that just died.
   assert(self:_in_state(SWITCHING)) 
   local actions = self._action_index[self._name]
   if actions then
      local entry = actions[current_active_key]
      self:_add_execution_unit(current_active_key, entry)
      -- there's no need to start it thinking.  we're switching to a new unit, so
      -- this one can't possibly be good for anything (at least not before we
      -- exit running or kill that other thing...)
   end

   -- switch_step(8) - yay!  now we can finally switch to the other unit. way back at step
   -- 1 we stashed that away.  take it out now!
   assert(self._switch_to_unit)
   assert(self._switch_to_unit:get_state() == 'ready')
   self._active_unit, self._switch_to_unit = self._switch_to_unit, nil
   self._active_unit:send_msg('start')
   self._thread:set_thread_data('stonehearth:unwind_to_frame', nil)
end

function ExecutionFrame:_add_action_from_stopped(key, entry)
   self:_add_execution_unit(key, entry)
end
  
function ExecutionFrame:_add_action_from_thinking(key, entry)
   local unit = self:_add_execution_unit(key, entry)
   unit:send_msg('start_thinking', self:_clone_entity_state('new speculation for unit'))
end

function ExecutionFrame:_add_action_from_ready(key, entry)
   self:_add_execution_unit(key, entry)
   -- we could kick it in the pants now, but let's not bother...
end

function ExecutionFrame:_remove_action_from_stopped(key, entry)
   local unit = self._execution_units[key]
   unit:send_msg('destroy')   
   self:_remove_execution_unit(unit)
end

function ExecutionFrame:_remove_action_from_finished(key, entry)
   local unit = self._execution_units[key]
   unit:send_msg('destroy')
   self:_remove_execution_unit(unit)
end

function ExecutionFrame:_remove_action_from_thinking(key, entry)
   local unit = self._execution_units[key]
   unit:send_msg('stop_thinking')
   unit:send_msg('destroy')   
   self:_remove_execution_unit(unit, true)
end

-- assumes the unit is already destroyed or aborted.  we just need to remove
-- it from our list
function ExecutionFrame:_remove_execution_unit(unit)
   assert(unit ~= self._active_unit)
   for key, u in pairs(self._execution_units) do
      if unit == u then
         self._log:debug('removing execution unit "%s"', unit:get_name())
         self._execution_units[key] = nil
         self._execution_unit_keys[unit] = nil
         return
      end
   end
end

function ExecutionFrame:_on_action_index_changed(add_remove, key, entry, does)
   if self._name == does then
      self:send_msg(add_remove .. '_action', key, entry)
   end
end

function ExecutionFrame:_add_execution_unit(key, entry)
   local name = tostring(type(key) == 'table' and key.name or key)
   self._log:debug('adding new execution unit: %s', name)
   
   local action
   local action_ctor = entry.action_ctor
   
   if action_ctor.create_action then
      action = action_ctor:create_action(self._entity, self._injecting_entity)
   else
      action = action_ctor(self._entity, self._injecting_entity)
   end
   local unit = ExecutionUnitV2(self,
                                self._thread,
                                self._debug_route,
                                self._entity,
                                entry.injecting_entity,
                                action,
                                self._args,
                                self._action_index)

   assert(not self._execution_units[key])
   self._execution_units[key] = unit
   self._execution_unit_keys[unit] = key
   return unit
end

function ExecutionFrame:_set_current_entity_state(state)
   self._log:debug('set_current_entity_state')
   assert(state)
   assert(self:get_state() == STOPPED)

   for key, value in pairs(state) do
      self._log:spam('  CURRENT.%s = %s', key, tostring(value))
   end
   
   -- we explicitly do not clone this state.  compound actions expect us
   -- to propogate side-effects of executing this frame back in the state
   -- parameter.
   self._current_entity_state = state
   self._saved_entity_state = self:_clone_entity_state('saved')
end

function ExecutionFrame:_clone_entity_state(name)
   assert(self._current_entity_state)
   local s = self._current_entity_state
   local cloned = {
      location = Point3(s.location.x, s.location.y, s.location.z),
      carrying = s.carrying,
   }
  self._log:spam('cloning current state %s to %s %s', tostring(self._current_entity_state), name, tostring(cloned)) 
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
      state = self:get_state(),
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
   self._sm:set_state(state)
   if self._ready_cb and state == READY then
      assert(self._active_unit_think_output)
      self._ready_cb(self, self._active_unit_think_output)
   end
end

function ExecutionFrame:_in_state(...)
   return self._sm:in_state(...)
end

function ExecutionFrame:get_state(state)
   return self._sm:get_state(state)
end

function ExecutionFrame:_protected_call(fn)
   local success, err = pcall(fn)
   if not success then
      if err:find(UNWIND_NEXT_FRAME_2) then
         self:_cleanup_protected_call_exit()      
         
         -- switch_step(5) - a frame called by the frame which needs to switch
         -- units has gracefully shutdown and returned early out of run.  we
         -- should still be on the thread for this frame.
         --[[
         assert(self:_in_state(SWITCHING, DEAD))
         assert(self._thread:is_running())
         assert(not self._active_unit)
         ]]

         -- switch_step(6) - but we don't know if we've made it all the way
         -- back to the frame we want to switch to.  check it out.  _unwind_call_stack
         -- *will not return*      
         self:_unwind_call_stack()
         assert(self._active_unit)
         self._log:debug('finished unwinding!')
      elseif err:find(UNWIND_NEXT_FRAME) then
         self:_cleanup_protected_call_exit()      

         -- it's magic.  don't question it (there's a 2nd pcall in run...
         self:_exit_protected_call(UNWIND_NEXT_FRAME_2)
      else
         assert(success, 'error in call?  now what?')
      end
   end
end

function ExecutionFrame:_exit_protected_call(sentinel)
   if decoda_break_on_error then
      decoda_break_on_error(false)
   end
   error(sentinel)
end

function ExecutionFrame:_cleanup_protected_call_exit(sentinel)
   if decoda_break_on_error then
      decoda_break_on_error(true)
   end
end

StateMachine.add_nop_state_transition(ExecutionFrame, 'stop', STOPPED)
StateMachine.add_nop_state_transition(ExecutionFrame, 'unit_stopped', STOPPED)

return ExecutionFrame
 