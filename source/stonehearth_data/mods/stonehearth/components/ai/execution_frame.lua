local ExecutionFrame = require 'components.ai.state_machine'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local ExecutionFrame = class()

local SET_ACTIVE_UNIT = { __comment = 'change active running unit' }

-- these values are in the interface, since we trigger them as events.  don't change them!
local STARTING_THINKING = 'starting_thinking'
local THINKING = 'thinking'
local READY = 'ready'
local STARTING = 'starting'
local STARTED = 'started'
local RUNNING = 'running'
local FINISHED = 'finished'
local STOPPING = 'stopping'
local STOPPED = 'stopped'
local DEAD = 'dead'

local ABORT_FRAME = ':aborted_frame:'
local UNWIND_NEXT_FRAME = ':unwind_next_frame:'
local UNWIND_NEXT_FRAME_2 = ':unwind_next_frame_2:'

function ExecutionFrame:__init(thread, debug_route, entity, activity_name, action_index)
   self._id = stonehearth.ai:get_next_object_id()
   self._debug_route = debug_route .. ' f:' .. tostring(self._id)
   self._entity = entity
   self._activity_name = activity_name
   self._action_index = action_index
   self._thread = thread
   self._execution_units = {}
   self._execution_unit_keys = {}
   self._saved_think_output = {}
   self._runnable_unit_count = 0

   local prefix = string.format('%s (%s)', self._debug_route, self._activity_name)
   self._log = radiant.log.create_logger('ai.exec_frame')
   self._log:set_prefix(prefix)
   self._log:debug('creating execution frame')
   
   self:_set_state(STOPPED)

   self:_create_execution_units()
   self._ai_component = entity:get_component('stonehearth:ai')
   radiant.events.listen(self._ai_component, 'stonehearth:action_index_changed:' .. self._activity_name, self, self._on_action_index_changed)

   if activity_name == 'stonehearth:top' then
      radiant.events.listen(entity, 'stonehearth:carry_block:carrying_changed', self, self._on_carrying_changed)
      self._position_trace = radiant.entities.trace_location(self._entity, 'find path to entity')
                                                :on_changed(function()
                                                   self:_on_position_changed()
                                                end)
   end
end

function ExecutionFrame:get_id()
   return self._id
end

function ExecutionFrame:_unknown_transition(msg)
   local err = string.format('bad frame transition "%s" from "%s"', msg, self._state)
   self._log:info(err)
   error(err)
end

function ExecutionFrame:_on_position_changed()
   if self._last_captured_location then
      local distance = radiant.entities.distance_between(self._entity, self._last_captured_location)
      if distance > 3 then
         self._log:detail('entity moved!  going to restart thinking... (state:%s)', self._state)
         self:_restart_thinking()
      end
   end
end

function ExecutionFrame:_on_carrying_changed()
   self._log:detail('carrying changed!  going to restart thinking... (state:%s)', self._state)
   self:_restart_thinking()
end


function ExecutionFrame:_start_thinking(args, entity_state)
   assert(args, "_start_thinking called with no arguments.")

   if self._state == 'stopped' then
      return self:_start_thinking_from_stopped(args, entity_state)
   end
   self:_unknown_transition('start_thinking')
end

function ExecutionFrame:_stop_thinking()
   if self:in_state('thinking', 'starting_thinking') then
      return self:_stop_thinking_from_thinking()
   end
   if self._state == 'ready' then
      return self:_stop_thinking_from_ready()
   end
   if self:in_state('starting', 'started') then
      return self:_stop_thinking_from_started() -- intentionally aliased
   end
   if self:in_state('stopped', 'dead') then
      return -- nop
   end
   self:_unknown_transition('stop_thinking')
end

function ExecutionFrame:_start()
   if self._state == 'ready' then
      return self:_start_from_ready()
   end
   if self._state == 'thinking' then
      return self:_start_from_thinking()
   end
   self:_unknown_transition('start')
end

function ExecutionFrame:_run()
   if self._state == 'started' then
      return self:_run_from_started()
   end
   self:_unknown_transition('run')
end

function ExecutionFrame:_unit_ready(unit, think_output)
   self._log:spam('_unit_ready (unit:"%s" state:%s)', unit:get_name(), self._state)
   if self._state == 'starting_thinking' then
      return self:_unit_ready_from_starting_thinking(unit, think_output)
   end
   if self._state == 'thinking' then
      return self:_unit_ready_from_thinking(unit, think_output)
   end
   if self._state == 'ready' then
      return self:_unit_ready_from_ready(unit, think_output)
   end
   if self._state == 'started' then
      return self:_unit_ready_from_started(unit, think_output)
   end
   if self._state == 'running' then
      return self:_unit_ready_from_running(unit, think_output)
   end
   if self:in_state('switching', 'finished', 'stopped') then
      return -- nop
   end
   if self._state == 'dead' then
      return self:_unit_ready_from_dead(unit, think_output)
   end

   self:_unknown_transition('unit_ready')
end

function ExecutionFrame:_unit_not_ready(unit)
   if self._state == 'thinking' then
      return self:_unit_not_ready_from_thinking(unit)
   end
   if self._state == 'ready' then
      return self:_unit_not_ready_from_ready(unit)
   end
   if self._state == 'dead' then
      return -- nop
   end
   self:_unknown_transition('unit_not_ready')
end

function ExecutionFrame:_stop()
   if self:in_state('ready', 'starting', 'started') then
      return self:_stop_from_started()
   end
   if self._state == 'thinking' then
      return self:_stop_from_thinking()
   end
   if self._state == 'finished' then
      return self:_stop_from_finished()
   end
   if self._state == 'running' then
      return self:_stop_from_running()
   end
   if self._state == 'stopped' then
      return -- nop
   end
   if self._state == 'dead' then
      return -- nop
   end
   self:_unknown_transition('stop')
end

function ExecutionFrame:_destroy()
   self._log:detail('_destroy (state:%s)', self._state)
   
   if self:in_state('starting', 'started', 'ready', 'starting_thinking', 'thinking') then
      return self:_destroy_from_starting()
   end
   if self._state == 'running' then
      return self:_destroy_from_running()
   end
   if self:in_state('stopped', 'finished') then
      return self:_destroy_from_stopped()
   end
   if self._state == 'dead' then
      return -- nop
   end
   self:_unknown_transition('destroy')
end

function ExecutionFrame:_add_action(...)
   if self._state == 'thinking' then
      return self:_add_action_from_thinking(...)
   end
   if self._state == 'ready' then
      return self:_add_action_from_ready(...)
   end
   if self._state == 'running' then
      return self:_add_action_from_running(...)
   end
   if self:in_state('stopped', 'switching', 'finished') then
      return self:_add_action_from_stopped(...)
   end
   self:_unknown_transition('add_action')
end

function ExecutionFrame:_remove_action(unit)
   if self._state == 'thinking' then
      return self:_remove_action_from_thinking(unit)
   end
   if self._state == 'ready' then
      return self:_remove_action_from_ready(unit)
   end
   if self:in_state('running', 'switching', 'starting') then
      return self:_remove_action_from_running(unit)
   end
   if self._state == 'stopping' then
      return self:_remove_action_from_stopping(unit)
   end
   if self._state == 'stopped' then
      return self:_remove_action_from_stopped(unit)
   end
   if self._state == 'finished' then
      return self:_remove_action_from_finished(unit)
   end
   self:_unknown_transition('remove_action')
end

function ExecutionFrame:_restart_thinking(entity_state)
   self._log:detail('_restart_thinking (state:%s)', self._state)

   if not self:in_state('thinking', 'starting_thinking', 'ready', 'running') then
      return
   end

   if entity_state then
      self:_set_current_entity_state(entity_state)
   else
      self:_capture_entity_state()
   end

   -- see which units we can restart and clone the state for them
   local rethinking_units = {}
   for _, unit in pairs(self._execution_units) do
      if self:_is_strictly_better_than_active(unit) then
         rethinking_units[unit] = self:_clone_entity_state('new speculation for unit')
      end
   end

   local current_state = self._state
   for unit, entity_state in pairs(rethinking_units) do
      self._log:detail('calling start_thinking on unit "%s" (unit state:%s).', unit:get_name(), unit:get_state())
      assert(unit:in_state('stopped', 'thinking', 'ready'))
      unit:_start_thinking(self._args, entity_state)

      -- if units bail or abort, the current pcall should have been interrupted.
      -- verify that this is so
      assert(self._state == current_state)
   end

   -- if anything became ready during the think, use it immediately.
   local unit = self:_get_best_execution_unit()
   if unit and unit ~= self._active_unit then
      self._log:detail('%s -> %s', self._active_unit and self._active_unit:get_name() or '', unit:get_name())
      local think_output = self._saved_think_output[unit]
      assert(think_output)
      self._log:detail('using "%s" as active unit in _restart_thinking.', unit:get_name())
      self:_set_active_unit(unit, think_output)
      self:_set_state(READY)
      return true
   end   
end

function ExecutionFrame:_start_thinking_from_stopped(args, entity_state)   
   self._log:debug('_start_thinking_from_stopped')
   
   self._args = args
   self._log:debug('set execution frame arguments to %s %s', self._activity_name, stonehearth.ai.format_args(self._args))

   self._runnable_unit_count = 0
   self:_set_state(STARTING_THINKING)
   if not self:_restart_thinking(entity_state) then
      self:_set_state(THINKING)
   end
end

function ExecutionFrame:_do_destroy()
   if self._position_trace then
      self._position_trace:destroy()
      self._position_trace = nil
   end
   radiant.events.listen(self._ai_component, 'stonehearth:action_index_changed:' .. self._activity_name, self, self._on_action_index_changed)
   radiant.events.unlisten(self._entity, 'stonehearth:carry_block:carrying_changed', self, self._on_carrying_changed)
end

function ExecutionFrame:start_thinking(args, entity_state)
   self._log:spam('start_thinking (state: %s)', self._state)
   assert(args, "start_thinking called with no arguments")
   assert(self:get_state() == STOPPED, string.format('start thinking called from non-stopped state "%s"', self:get_state()))

   self:_start_thinking(args, entity_state)
   if self:in_state(READY) then
      assert(self._active_unit_think_output)
      return self._active_unit_think_output
   end
   self._log:spam('not immediately ready after start_thinking.')
end

function ExecutionFrame:on_ready(ready_cb)
   self._log:spam('registering on_ready callback (state: %s)', self._state)
   if not ready_cb then
      self._ready_cb = nil
      return
   end

   local calling_thread = stonehearth.threads:get_current_thread()
   self._ready_cb = function(think_output)
         local arg = think_output and stonehearth.ai:format_args(think_output) or 'nil'

         if calling_thread and not calling_thread:is_running() then
            -- this interrupt is synchronous to make sure we deliver the callback in the current.
            -- state.  for example, soon after we return from this function, a task might kill
            -- an action that's running this frame (ending up stopping the frame before the 
            -- thread which wants the on_ready() notification actually gets scheduled!)
            self._log:detail('ai thread is not running trying to deliver on_ready(%s) notification.  thunking.', arg)
            calling_thread:interrupt(function()
                  assert(self._ready_cb)
                  self._log:detail('calling on_ready(%s) notification with thunk', arg)
                  ready_cb(self, think_output)
               end)
         else
            -- sometimes there is no thread (e.g. in pathfinder callbacks).
            -- just use whatever thread we're on.
            self._log:detail('calling on_ready(%s) notification without thunk', arg)
            ready_cb(self, think_output)
         end
      end
end

function ExecutionFrame:stop_thinking()
   -- stop thinking can be called from any thread.  make sure we get back to 
   -- the main thread before executing (in case we need to unwind the stack)
   self._log:spam('stop_thinking')
   self:_protected_call(function()
         self:_stop_thinking()
      end)
end

function ExecutionFrame:start()
   assert(self:_no_other_thread_is_running())

   self._log:spam('start')
   self:_protected_call(function()
         self:_start()
         self:wait_until(STARTED)
      end)
end

function ExecutionFrame:run(args)
   assert(self._thread:is_running())

   self._log:spam('run')

   local run_stack = self._thread:get_thread_data('stonehearth:run_stack')
   table.insert(run_stack, self)  
   self:_log_stack('making pcall')

   local call_fn = function()
      if not self:in_state(STOPPED, READY, STARTED) then
         error(string.format('invalid initial state "%s" in run', self:get_state()))
      end
      
      if self:in_state(STOPPED) then
         assert(args, "run from stopped state must pass in arguments!")
         self:_start_thinking(args)
         self:wait_until(READY)
      end
      if self:in_state(READY) then
         self:_start()
         self:wait_until(STARTED)
      end
      assert(self:in_state(STARTED))
      --self:_stop_thinking()
      assert(self:in_state(STARTED))
      
      self:_run()
      self:wait_until(FINISHED)
   end   
   local call_exit_fn = function()
      self:_log_stack('pcall completed')
      local last_frame = table.remove(run_stack)
      assert(run_stack == self._thread:get_thread_data('stonehearth:run_stack'))
      assert(self == last_frame)
   end
   
   self:_protected_call(call_fn, call_exit_fn)
end

function ExecutionFrame:stop()
   assert(self:_no_other_thread_is_running())

   self._log:spam('stop (state:%s)', self._state)
   if self._state ~= DEAD then
      self:_protected_call(function()   
            self:_stop()
            self:wait_until(STOPPED)
         end)
   end
end

function ExecutionFrame:destroy()
   self._log:spam('destroy')
   self:_protected_call(function()   
         self:_destroy()
         self:wait_until(DEAD)
      end)
end

function ExecutionFrame:_create_execution_units()
   local actions = self._action_index[self._activity_name]
   if not actions then
      self._log:warning('no actions for %s at the moment.  this is actually ok for tasks (they may come later)', self._activity_name)
      return
   end
   for key, entry in pairs(actions) do
      self:_add_execution_unit(key, entry)
   end
end

function ExecutionFrame:_stop_thinking_from_thinking()
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:_stop_thinking()
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_thinking_from_ready()
   self._log:debug('_stop_thinking_from_ready (state: %s)', self._state)

   for _, unit in pairs(self._execution_units) do
      unit:_stop_thinking()
   end
   self:_set_active_unit(nil)
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_thinking_from_started()
   self._log:debug('_stop_thinking_from_started')
   assert(self._active_unit)
   
   for _, unit in pairs(self._execution_units) do
      if unit ~= self._active_unit and not self:_is_strictly_better_than_active(unit) then
         unit:_stop_thinking() -- this guy has no prayer...  just stop
      else
         -- let better ones keep processing.  they may prempt in the future
      end
   end
end

function ExecutionFrame:_do_unit_ready_bookkeeping(unit, think_output)
   -- remember the think output.  the frame caller up the stack will
   -- use it during the election, later.
   assert(think_output)
   self._log:detail('setting think output for %s to %s', unit:get_name(), tostring(think_output))
   self._saved_think_output[unit] = think_output
   self._runnable_unit_count = self._runnable_unit_count + 1
end

function ExecutionFrame:_unit_ready_from_starting_thinking(unit, think_output)
   self:_do_unit_ready_bookkeeping(unit, think_output)
end

function ExecutionFrame:_unit_ready_from_thinking(unit, think_output)
   self:_do_unit_ready_bookkeeping(unit, think_output)

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
   self:_do_unit_ready_bookkeeping(unit, think_output)

   if self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s"',
                       self._active_unit:get_name(), unit:get_name())
   else
      return
   end
   self:_set_active_unit(unit, think_output)
end

function ExecutionFrame:_unit_ready_from_started(unit, think_output)
   assert(self._active_unit)
   -- so... someone called start(), we picked an execution unit and called start()
   -- on it.  before the original caller could call run(), *another* unit became
   -- ready.  if that one is better than the one we picked before, we can just
   -- switch to do it (no harm, no foul)
   self:_do_unit_ready_bookkeeping(unit, think_output)

   if self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s"',
                       self._active_unit:get_name(), unit:get_name())
      assert(self._active_unit)
      
      -- we started it, so we'd best stop it before replacing it.
      self._active_unit:_stop()
   else
      return
   end
   self:_set_active_unit(unit, think_output)
end

function ExecutionFrame:_unit_ready_from_running(unit, think_output)
   self._log:detail('_unit_ready_from_running (%s)', unit:get_name())
   assert(self._active_unit)

   self:_do_unit_ready_bookkeeping(unit, think_output)
   
   if self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s" while running!!',
                       self._active_unit:get_name(), unit:get_name())

      -- how do we switch to a different unit?  follow the bouncing ball --> switch_step(...).

      assert(self._state == RUNNING)
      assert(not self._switch_to_unit)
      self._switch_to_unit = unit
      
      -- switch_step(2) - put an interrupt handler on the thread for this frame (which,
      -- remember, is *not* currently executing) to rewind the stack all the way back
      -- to the current frame
      assert(not self._thread:get_thread_data('stonehearth:unwind_to_frame'))
      self._thread:set_thread_data('stonehearth:unwind_to_frame', self)
      self._log:detail('thunking back to run thread to unwind stack.')
      self._thread:interrupt(function()
            local unwind_frame = self._thread:get_thread_data('stonehearth:unwind_to_frame')
            assert(unwind_frame, 'entered unwind frame interrupt with no unwind frame!')
            self._log:detail('starting unwind to frame %s', unwind_frame._activity_name)
            
            -- make sure we unwind the frame at the top of the stack.  'self' is the
            -- frame that wants to get ready, which is way up the stack.
            self:_exit_protected_call(UNWIND_NEXT_FRAME_2)
         end)
   else
      -- not better than our current unit?  just ignore him
   end
end

function ExecutionFrame:_unit_ready_from_ready(unit, think_output)
   assert(self._active_unit)
   self:_do_unit_ready_bookkeeping(unit, think_output)

   if self:_is_strictly_better_than_active(unit) then
      self._log:detail('replacing unit "%s" with better active unit "%s"',
                       self._active_unit:get_name(), unit:get_name())
   else
      return
   end
   self:_set_active_unit(unit, think_output)
end

function ExecutionFrame:_unit_ready_from_dead(unit, think_output)
   self._log:debug('ignoring ready message while dead')
   assert(#self._execution_units == 0, 'frame still has units while dead')
end

function ExecutionFrame:_unit_not_ready_from_thinking(unit)
   self._log:detail('_unit_not_ready_from_thinking (%s)', unit:get_name())
   if unit == self._active_unit then
      self._log:detail('best choice for active_unit "%s" became unready...', unit:get_name())
      self:_find_replacement_for_active_unit()
   end
end

function ExecutionFrame:_unit_not_ready_from_ready(unit)
   self._log:detail('_unit_not_ready_from_ready (%s)', unit:get_name())
   if unit == self._active_unit then
      self._log:debug('active unit became unready.  going back to thinking')
      self:_set_active_unit(nil)
      self:_set_state(THINKING)
      if self._ready_cb then
         self._log:debug('sending unready notification')
         self._ready_cb(nil)
      end
   end
end

function ExecutionFrame:_set_active_unit(unit, think_output)
   local aname = self._active_unit and self._active_unit:get_name() or 'none'
   local uname = unit and unit:get_name() or 'none'
   self._log:detail('replacing active unit "%s" with "%s"', aname, uname)
   
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
   if unit then
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
   self._active_unit:_start()
   self:_set_state(STARTED)
end

function ExecutionFrame:_start_from_thinking()
   -- this happens sometimes when we lose a race.  it looks something like this:
   -- 1) we become ready, but the owning task won't let us actually run for a little bit.
   -- 2) simultaneously, something causes us to become unready and go back to thinking and
   --    our task decides it's ok for us to go.
   -- in this case, just abort ignor it
   self._log:detail('ignoring start from thinking state.')
end

function ExecutionFrame:_run_from_started()
   assert(self._active_unit)

   self:_set_state(RUNNING)

   local finished, running_unit
   repeat
      running_unit = self._active_unit
      self:_protected_call(function()
            running_unit:_run()
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

function ExecutionFrame:_stop_from_started()
   assert(self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:_stop()
   end
   self:_set_active_unit(nil)
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_from_thinking()
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:_stop_thinking()
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_from_running()
   -- stop from running can only happen if there are no other threads running.
   -- this can occur when an entity is destroyed from a C callback into the
   -- game engine.  if we were running at the time, we'll take the standard
   -- unwind pcall path where we simply get destroyed
   assert(self._thread:is_running() or stonehearth.threads:get_current_thread() == nil)
   assert(self._active_unit)

   -- move into the stopping state, so we can take extra special care when stopping.
   -- for example, somewhere deep below we may stop a RunActionTask, which will
   -- ask it's parent Task to remove the action from the index.  Normally removing an
   -- action from the index while it's currently running forces an abort, but we
   -- certainly don't want to do that here!
   self:_set_state(STOPPING)
   self:_set_active_unit(nil)
   for _, unit in pairs(self._execution_units) do
      unit:_stop()
   end
   self:_set_state(STOPPED)
end
   
function ExecutionFrame:_stop_from_finished()
   self._log:detail('_stop_from_finished')
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      self._log:detail('stopping execution unit "%s"', unit:get_name())
      unit:_stop(true)
   end
   self:_set_state(STOPPED)
end

function ExecutionFrame:_destroy_from_starting()
   self._log:debug('_destroy_from_starting (state:%s)', self._state)
   for _, unit in pairs(self._execution_units) do
      if unit == self._active_unit then
         unit:_stop()
      end
      unit:_stop_thinking()
      unit:_destroy()
   end
   self._execution_units = {}
   self._execution_unit_keys = {}
   self:_set_state(DEAD)
   self:_do_destroy()
end

function ExecutionFrame:_destroy_from_stopped()
   self._log:debug('_destroy_from_stopped (state: %s)', self._state)
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:_destroy()
   end
   self._execution_units = {}
   self._execution_unit_keys = {}
   self:_set_state(DEAD)
   self:_do_destroy()
end


function ExecutionFrame:_destroy_from_running()
   -- verify either we're the thread running or we've been destroyed from a C callback
   assert(self:_no_other_thread_is_running())
   
   for _, unit in pairs(self._execution_units) do
      unit:_destroy()
   end
   self._execution_units = {}
   self._execution_unit_keys = {}
   self:_set_state(DEAD)
   self:_do_destroy()
end

function ExecutionFrame:abort()
   self._log:debug('abort')
   assert(not self._aborting)
   self._aborting = true
   assert(self:_no_other_thread_is_running())
   self:_exit_protected_call(ABORT_FRAME)
end

function ExecutionFrame:_get_top_of_stack()
   local runstack = self._thread:get_thread_data('stonehearth:run_stack')
   return runstack[#runstack]
end

function ExecutionFrame:_log_stack(msg)
   self._log:detail(msg)
   local runstack = self._thread:get_thread_data('stonehearth:run_stack')
   for i = #runstack,1,-1 do
      local f = runstack[i]
      self._log:spam('  [%d] - (%5d) %s', i, f._id, f._activity_name, stonehearth.ai.format_args(f._args))
   end
end

function ExecutionFrame:_unwind_call_stack(exit_handler)  
   local going_to_frame = self._thread:get_thread_data('stonehearth:unwind_to_frame')
   assert(going_to_frame)

   self:_log_stack(string.format('_unwind_call_stack %s', going_to_frame._activity_name))
   local current_run_frame = self:_get_top_of_stack()

   assert(current_run_frame)
   assert(current_run_frame == self)
   assert(self._thread:is_running())   

   -- switch_step(4) - unfortunately, we may be really really deep in nested calls
   -- which all need to be unwound before we can switch.  unwind them all first.
   if current_run_frame ~= going_to_frame then
      -- if the next frame is where we want to go, only unwind 1 frame so we
      -- end up back in the protected run loop in :_run_from_*.  otherwise,
      -- jump back 2 frames
      self._log:detail('still unwinding... aborting current pcall')
      exit_handler()
      local top = self:_get_top_of_stack()
      if top ~= going_to_frame then
         self:_exit_protected_call(UNWIND_NEXT_FRAME)
      else
         self:_exit_protected_call(UNWIND_NEXT_FRAME_2)
      end
      radiant.not_reached()
   end

   self._log:detail('reached unwind frame!... starting new unit')
   self._thread:set_thread_data('stonehearth:unwind_to_frame', nil)
   
   -- switch_step(7) - we have finally unwound all our called frames (stopping them
   -- along the way), and have reached the switching frame!
   assert(self:in_state(RUNNING))
   assert(self._switch_to_unit)
   assert(self._switch_to_unit:get_state() == 'ready')
   self._active_unit, self._switch_to_unit = self._switch_to_unit, nil

   -- stop everything but the active unit
   for _, unit in pairs(self._execution_units) do
      if unit ~= self._active_unit then
         unit:_stop(true)
      end
   end
   -- start the active unit and start the rest of them thinking again.
   self._active_unit:_start()
   self:_restart_thinking()
end
 
function ExecutionFrame:_add_action_from_thinking(key, entry)
   local unit = self:_add_execution_unit(key, entry)
   unit:_start_thinking(self._args, self:_clone_entity_state('new speculation for unit'))
end

function ExecutionFrame:_add_action_from_ready(key, entry)
   self:_add_execution_unit(key, entry)
   -- we could kick it in the pants now, but let's not bother...
end

function ExecutionFrame:_add_action_from_running(key, entry)
   local unit = self:_add_execution_unit(key, entry)
   unit:_start_thinking(self._args, self:_create_entity_state())
end

function ExecutionFrame:_add_action_from_stopped(key, entry)
   self._log:detail('_add_action_from_stopped (state: %s)', self._state)
   self:_add_execution_unit(key, entry)
end

function ExecutionFrame:_remove_action_from_stopped(unit)
   self:_remove_execution_unit(unit)
end

function ExecutionFrame:_remove_action_from_stopping(unit)
   self:_remove_execution_unit(unit)
end

function ExecutionFrame:_remove_action_from_finished(unit)
   self:_remove_execution_unit(unit)
end

function ExecutionFrame:_remove_action_from_thinking(unit)
   self:_remove_execution_unit(unit)
end

function ExecutionFrame:_remove_action_from_ready(unit)
   if unit == self._active_unit then
      self._active_unit:_stop_thinking()
      self:_set_active_unit(nil)
      if self._ready_cb then
         self._log:debug('sending unready notification')
         self._ready_cb(nil)
      end     
      self:_set_state(STOPPED)
      self:_restart_thinking(self._current_entity_state)
   else
      self:_remove_execution_unit(unit)
   end
end

function ExecutionFrame:_remove_action_from_running(unit)
   self._log:detail('_remove_action_from_running (state: %s)', self._state)
   if unit == self._active_unit then
      if not self._aborting then
         self._thread:interrupt(function()
            self:abort()
         end)
      end
   else
      self:_remove_execution_unit(unit)
   end
end

-- assumes the unit is already destroyed or aborted.  we just need to remove
-- it from our list
function ExecutionFrame:_remove_execution_unit(unit)
   assert(unit)
   assert(unit ~= self._active_unit)
   for key, u in pairs(self._execution_units) do
      if unit == u then
         self._log:debug('removing execution unit "%s"', unit:get_name())
         unit:_destroy()
         self._execution_units[key] = nil
         self._execution_unit_keys[unit] = nil
         return
      end
   end
end

function ExecutionFrame:_on_action_index_changed(add_remove, key, entry, does)
   if self._log:is_enabled(radiant.log.DETAIL) then
      local key_name
      if type(key) == 'table' and key.name then
         key_name = 'dynamic_action:' .. key.name
      else
         key_name = tostring(key_name)
      end
      self._log:debug('on_action_index_changed %s (key:%s state:%s)', add_remove, key_name, self._state)
   end
   if self._activity_name == does and self:get_state() ~= DEAD then
      local unit = self._execution_units[key]
      if add_remove == 'add' then
         if not unit then         
            self:_add_action(key, entry)
         end
      elseif add_remove == 'remove' then
         if unit then         
            self:_remove_action(unit)
         end
      else
         error(string.format('unknown msg "%s" in _on_action_index-changed', add_move))
      end
   end
end

function ExecutionFrame:_add_execution_unit(key, entry)
   local name = tostring(type(key) == 'table' and key.name or key)
   self._log:debug('adding new execution unit: %s', name)

   if not entry then
      local actions = self._action_index[self._activity_name]
      if not actions then
         self._log:warning('no actions for %s at the moment.  this is actually ok for tasks (they may come later)', self._activity_name)
         return
      end
      entry = actions[key]
      assert(entry)
   end

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
                                self._action_index)

   assert(not self._execution_units[key])
   self._execution_units[key] = unit
   self._execution_unit_keys[unit] = key
   return unit
end

function ExecutionFrame:_set_current_entity_state(state)
   self._log:debug('set_current_entity_state')
   assert(state)

   for key, value in pairs(state) do
      self._log:spam('  CURRENT.%s = %s', key, tostring(value))
   end

   -- remember where we think we are so we can restart thinking if we
   -- move too far away from it.
   self._last_captured_location = state.location
   
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

function ExecutionFrame:_create_entity_state()
   local state = {
      location = radiant.entities.get_world_grid_location(self._entity),
      carrying = radiant.entities.get_carrying(self._entity),
   }
   return state
end

function ExecutionFrame:_capture_entity_state()
   self._log:debug('_capture_entity_state')
   
   local state = self:_create_entity_state()
   self:_set_current_entity_state(state)
end

-- *strictly* better.  no run-offs for latecomers.
function ExecutionFrame:_is_strictly_better_than_active(unit)
   if not self._active_unit then
      self._log:spam('  no active_unit.  "%s" is better!', unit:get_name())
      return true
   end
   if unit == self._active_unit then
      self._log:spam('  unit "%s" is active unit, therefore is not better!', unit:get_name())
      return false
   end

   local unit_priority = unit:get_priority()
   local active_priority = self._active_unit:get_priority()

   if unit_priority > active_priority then
      self._log:spam('  unit %s priority %d > active unit "%s" priority %d.  therefore is better!',
                     unit:get_name(), unit_priority, self._active_unit:get_name(), active_priority)
      return true
   elseif unit_priority < active_priority then
      self._log:spam('  unit %s priority %d < active unit "%s" priority %d.  therefore is not better!',
                     unit:get_name(), unit_priority, self._active_unit:get_name(), active_priority)
      return false
   end

   -- if they're exactly the same, the odds of replacing the current unit
   -- with this other one are the equal (proportional the number of other
   -- things we've seen roll by). (xxx: technically, the odds should be a 
   -- function of the combined weights of all the units of this priority
   -- which are currently ready)
   local better = rng:get_int(1, self._runnable_unit_count) == self._runnable_unit_count
   self._log:spam('  unit "%s" and active unit "%s" both have priority %d.  tossing a coin...',
                  unit:get_name(), self._active_unit:get_name(), unit_priority)
   return better
end

function ExecutionFrame:_get_best_execution_unit()
   local best_priority = 0
   local active_units = nil
   
   self._log:spam('%s looking for best execution unit for "%s"', self._entity, self._activity_name)
   for _, unit in pairs(self._execution_units) do
      local name = unit:get_name()
      local priority = unit:get_priority()
      local is_runnable = unit:is_runnable()
      self._log:spam('  unit %s has priority %d (weight: %d  runnable: %s  state:%s)',
                     name, priority, unit:get_weight(), tostring(is_runnable), unit:get_state())
      if is_runnable then
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
   self._log:spam('%s  best unit for "%s" is "%s" (priority: %d)', self._entity, self._activity_name, active_unit:get_name(), best_priority)
   
   return active_unit
end

function ExecutionFrame:get_debug_info()
   local info = {
      id = 'f:' .. tostring(self._id),
      state = self:get_state(),
      activity = {
         name = self._activity_name,
      },
      args = stonehearth.ai:format_args(self._args),
      execution_units = {
         n = 0,
      }
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


function ExecutionFrame:in_state(...)
   local states = { ... }
   for _, state in ipairs(states) do
      if self._state == state then
         return true
      end
   end
   return false
end

function ExecutionFrame:get_state()
   return self._state
end

function ExecutionFrame:_set_state(state)
   self._log:debug('state change %s -> %s', tostring(self._state), state)
   self._state = state
   
   local going_to_frame = self._thread:get_thread_data('stonehearth:unwind_to_frame')
   assert(not going_to_frame)
 
   if self._state == self._waiting_until then
      self._calling_thread:resume('transitioned to ' .. self._state)
   end
   if self._ready_cb and state == READY then
      assert(self._active_unit)
      assert(self._active_unit_think_output)
      self._ready_cb(self._active_unit_think_output)
   end
end

function ExecutionFrame:wait_until(state)
   self._calling_thread = stonehearth.threads:get_current_thread()   
   if self._state ~= state then
      self._waiting_until = state
      while self._state ~= state do
         self._calling_thread:suspend('waiting for ' .. state)
      end
      self._waiting_until = nil
      self._calling_thread = nil
   end
end

function ExecutionFrame:_protected_call(fn, exit_handler)
   exit_handler = exit_handler or function() end
   local error_handler = function(err)
      if err:find(UNWIND_NEXT_FRAME_2) then
         return UNWIND_NEXT_FRAME_2
      elseif err:find(UNWIND_NEXT_FRAME) then
         return UNWIND_NEXT_FRAME
      elseif err:find(ABORT_FRAME) then
         return ABORT_FRAME
      end
      
      local traceback = debug.traceback()
      _host:report_error(err, traceback)
      return err
   end
   local success, err = xpcall(fn, error_handler)
   if not success then
      if err == UNWIND_NEXT_FRAME_2 then
         self:_cleanup_protected_call_exit()
         self:_log_stack('in UNWIND_NEXT_FRAME_2 handler...')
         
         -- switch_step(5) - a frame called by the frame which needs to switch
         -- units has gracefully shutdown and returned early out of run.  we
         -- should still be on the thread for this frame.
         --[[
         assert(self:in_state(SWITCHING, DEAD))
         assert(self._thread:is_running())
         assert(not self._active_unit)
         ]]

         -- switch_step(6) - but we don't know if we've made it all the way
         -- back to the frame we want to switch to.  check it out.  _unwind_call_stack
         -- *will not return*      
         self:_unwind_call_stack(exit_handler)
         assert(self._active_unit)
         self._log:debug('finished unwinding!')
      elseif err == UNWIND_NEXT_FRAME then
         self:_cleanup_protected_call_exit()
         exit_handler()
         self:_log_stack('in UNWIND_NEXT_FRAME handler...')         
         -- it's magic.  don't question it (there's a 2nd pcall in run...
         self:_exit_protected_call(UNWIND_NEXT_FRAME_2)
      else
         self._log:info("aborting on error '%s' (state:%s)", err, self._state)
         self:_stop(true)
         assert(self._state == STOPPED, string.format('stop during abort took us to non-stopped state "%s"', self._state))
         self:_cleanup_protected_call_exit()
         exit_handler()
         
         -- we're aborting... hrm.  unwind all the way out
         local run_stack = self._thread:get_thread_data('stonehearth:run_stack')
         if #run_stack == 0 then
            return false
         end
         self:_exit_protected_call(err)
         radiant.not_reached()
      end
   end
   exit_handler()
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

function ExecutionFrame:_no_other_thread_is_running()
   return self._thread:is_running() or stonehearth.threads:get_current_thread() == nil
end

return ExecutionFrame

 