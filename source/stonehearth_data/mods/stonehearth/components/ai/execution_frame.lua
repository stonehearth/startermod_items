local ExecutionFrame = require 'components.ai.state_machine'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local ExecutionFrame = radiant.class()

-- errata note 1:  look at _restart_thinking()?  what crackhead wrote that (probably me).
-- issues:
--   1) silently nop's in most states.  why?  what if we really wanted to restart thinking
--      in those states?  we have to magically jump to another state that is valid to do
--      so (callers ACTAULLY do this!!).  that is b0rken.
--   2) takes us to the READY state when true is returned, but does nothing when returning
--      false.  it probably shouldn't take us to any state AT ALL and leave it up to the
--      caller to determine where it wants to go.
-- A better interafce would be to (perhaps...) simply return the most ready action, or
-- nothing at all and let the caller use the get_best_actino (or whatever) primatives or
-- something.... Anyway, needs much revisiting (see 'errata note 1' comments to find all
-- the suspect code)

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
local HALTED = 'halted'

local debug_info_state_order = {
   [RUNNING] = 0,
   [STARTING] = 1,
   [STARTED] = 1,
   [READY] = 2,
   [THINKING] = 3,
   [STARTING_THINKING] = 3,
   [FINISHED] = 9,
   [STOPPING] = 9,
   [STOPPED] = 9,
   [DEAD] = 9,
}

local EF_STATS = {

}

function ExecutionFrame._dump_and_reset_stats()
   local stats_copy = EF_STATS
   EF_STATS = {}
   _host:report_cpu_dump(stats_copy, 'ef_stats')
end

radiant.events.listen(radiant, 'radiant:report_cpu_profile', ExecutionFrame._dump_and_reset_stats)

-- the max roam distance is intentionally set really really really high to avoid
-- creating and re-creating many, many pathfinders while doing ambient behaviors
-- which may move the entity small distances (e.g. idle while bored) or paths
-- which are ultimately equivalent (e.g. pathfinders created for higher priority
-- tasks by a worker currently working on a wall).
--
local MAX_ROAM_DISTANCE = 32
local INFINITE = 1000000000
local ABORT_FRAME = ':aborted_frame:'
local UNWIND_NEXT_FRAME = ':unwind_next_frame:'
local UNWIND_NEXT_FRAME_2 = ':unwind_next_frame_2:'

local SLOWSTART_TIMEOUTS = {250, 750}
local SLOWSTART_MAX_UNITS = {4, INFINITE}
local SLOW_START_ENABLED = radiant.util.get_config('enable_ai_slow_start', true)
radiant.log.write('stonehearth', 0, 'ai slow start is %s', SLOW_START_ENABLED and "enabled" or "disabled")

-- only theee keys are allowed to be set.  all others are forbidden to 
-- prevent actions from using ai.CURRENT as a private communication channel
local ENTITY_STATE_FIELDS = {
   location = true,
   carrying = true,
   future = true,
}

local ENTITY_STATE_META_TABLE = {
   __newindex = function(state, k, v)
      if not ENTITY_STATE_FIELDS[k] then
         radiant.error('cannot write to invalid field "%s" in entity state', tostring(k))
      end

      if k == 'location' then
         -- if this is not the first time we're writing a new value to location, we
         -- must be in some future state.
         if state.__values.location and state.__values.location ~= v then
            state.__values.future = true
         end
      end
      state.__values[k] = v
   end,

   __index = function(state, k)
      return state.__values[k]
   end
}

local function create_entity_state(copy_from)
   -- we hide the actual values in __values so __new_index will catch every
   -- write.
   local state = {
      __values = {
         future = false
      }
   }
   if copy_from then
      for k, v in pairs(copy_from.__values) do
         state.__values[k] = v
      end
   end
   setmetatable(state, ENTITY_STATE_META_TABLE)

   return state
end

local function copy_entity_state(state, copy_from)
   -- we iterate through the keys rather than copying the whole table to make
   -- sure shared references among units get the changes as well
   for name in pairs(state.__values) do
      state.__values[name] = nil
   end
   for name, value in pairs(copy_from.__values) do
      state.__values[name] = value
   end
end

function ExecutionFrame:__init(thread, entity, action_index, activity_name, debug_route)
   self._id = stonehearth.ai:get_next_object_id()
   self._debug_route = debug_route .. ' f:' .. tostring(self._id)
   self._entity = entity
   self._activity_name = activity_name
   self._action_index = action_index
   self._thread = thread
   self._execution_units = {}
   self._execution_filters = {}
   self._saved_think_output = {}
   self._slow_start_timers = {}
   self._think_progress_cb = nil
   self._state = STOPPED
   self._rerun_unit = nil
   self._ai_component = entity:get_component('stonehearth:ai')

   local prefix = string.format('%s (%s)', self._debug_route, self._activity_name)
   self._log = radiant.log.create_logger('ai.exec_frame')
                          :set_prefix(prefix)
                          :set_entity(self._entity)

   self._log:debug('creating execution frame')

   self:_set_state(STOPPED)

   self:_create_execution_units()

   if activity_name == 'stonehearth:top' then
      self._carry_listener = radiant.events.listen(entity, 'stonehearth:carry_block:carrying_changed', self, self._on_carrying_changed)
      self._position_trace = radiant.entities.trace_location(self._entity, 'find path to entity')
                                                :on_changed(function()
                                                   self:_on_position_changed()
                                                end)
   end
end

function ExecutionFrame:get_id()
   return self._id
end

function ExecutionFrame:_is_top_idle()
   if self._active_unit then
      return self._active_unit:get_action().name == 'idle top'
   end
   return false
end

function ExecutionFrame:get_top_idle_state()
   local runstack = self._thread:get_thread_data('stonehearth:run_stack')
   local bottom_frame = runstack[1]
   if bottom_frame then
      local active_unit = bottom_frame._active_unit
      if active_unit then
         local action = active_unit:get_action()
         if action.name ~= 'idle top' then
            return active_unit:get_state()
         end
      end
   end
   -- if any of that up there failed to return an expected result, assume we're thinking
   return THINKING
end

function ExecutionFrame:_unknown_transition(msg)
   local err = string.format('bad frame transition "%s" from "%s"', msg, self._state)
   self._log:info(err)
   error(err)
end

function ExecutionFrame:_on_position_changed()
   if self._last_captured_location then
      local distance = radiant.entities.distance_between(self._entity, self._last_captured_location)
      if distance > MAX_ROAM_DISTANCE then
         -- we're currently on the script host callback thread servicing a trace...
         -- _restart_thinking does all sorts of nasty things to the stack (e.g. if a unit becomes ready
         -- in the middle of restart_thinking in the running state, it will try to unwind the stack and
         -- resume the new one).  Make sure we're on the AI thread before attempting that.
         self._thread:interrupt(function()
               self._log:detail('entity moved!  going to restart thinking... (state:%s)', self._state)
               -- errata note 1 : this is quite bizarre.  if we're running, we ESPECIALLY want to restart
               -- thinking, right?  unforutnately, this does NOTHING from the RUNNING state.
               self:_restart_thinking(nil, "entity moved")
            end)
      end
   end
end

function ExecutionFrame:_on_carrying_changed()
   -- we're currently on the main thread in a trigger callback...
   -- _restart_thinking does all sorts of nasty things to the stack (e.g. if a unit becomes ready
   -- in the middle of restart_thinking in the running state, it will try to unwind the stack and
   -- resume the new one).  Make sure we're on the AI thread before attempting that.
   self._thread:interrupt(function()
         self._log:detail('carrying changed!  going to restart thinking... (state:%s)', self._state)
         -- errata note 1 : this is quite bizarre.  if we're running, we ESPECIALLY want to restart
         -- thinking, right?  unforutnately, this does NOTHING from the RUNNING state.
         self:_restart_thinking(nil, "carrying changed")
      end)
end


function ExecutionFrame:_start_thinking(args, entity_state)
   assert(args, "_start_thinking called with no arguments.")

   if self._state == 'stopped' then
      return self:_start_thinking_from_stopped(args, entity_state)
   end
   self:_unknown_transition('start_thinking')
end

function ExecutionFrame:_stop_thinking()
   if self._state == 'thinking' or self._state == 'starting_thinking' then
      return self:_stop_thinking_from_thinking()
   end
   if self._state == 'ready' then
      return self:_stop_thinking_from_ready()
   end
   if self._state == 'starting' or self._state == 'started' then
      return self:_stop_thinking_from_started() -- intentionally aliased
   end
   if self._state == 'stopped' or self._state == 'dead' then
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

function ExecutionFrame:_unit_halted(unit, think_output)
   self._log:spam('_unit_halted (unit:"%s" state:%s)', unit:get_name(), self._state)
   if self._state == 'starting_thinking' then
      return self:_unit_halted_from_starting_thinking(unit, think_output)
   end
   self:_unknown_transition('unit_halted')
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
   if self:in_state('started', 'running') then
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
   if self:in_state('running', 'switching', 'starting', 'started') then
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


if radiant.util.get_config('enable_ef_stats', false) then
   function ExecutionFrame:_record_fast_think(units)
      local counts = EF_STATS[self._activity_name]
      if not counts then
         counts = {
            fast = {},
            slow = {},
            really_slow = {},
         }
         EF_STATS[self._activity_name] = counts
      end

      local unit_count = 0
      for _, __ in pairs(units) do
         unit_count = unit_count + 1
      end

      if unit_count > 10 then
         local data = {}
         for unit, _ in pairs(units) do
            table.insert(data, unit:get_action().name .. ':' .. unit:get_priority())
         end
         table.insert(counts.fast, data)
      end
   end

   function ExecutionFrame:_record_slow_think(timeout, unit)
      local countlist = EF_STATS[self._activity_name]
      if not countlist then
         countlist = {
            fast = {},
            slow = {},
            really_slow = {},
         }
         EF_STATS[self._activity_name] = countlist
      end
      local counts = countlist.slow
      if timeout == SLOWSTART_TIMEOUTS[2] then
         counts = countlist.really_slow
      end

      local count = counts[unit:get_action().name]
      if not count then
         counts[unit:get_action().name] = 1
      else
         counts[unit:get_action().name] = count + 1
      end
   end
else
   function ExecutionFrame:_record_slow_think(timeout, unit)
   end
   function ExecutionFrame:_record_fast_think(units)
   end
end
function ExecutionFrame:_destroy_slow_timers()
   for _, timer in pairs(self._slow_start_timers) do
      timer:destroy()
   end
   self._slow_start_timers = {}
end

function ExecutionFrame:_do_slow_thinking(local_args, units, units_start, num_units, timeout)
   -- If this frame is dead, return.
   if self._state == DEAD then
      return
   end

   -- If this frame is thinking, then we should think all units.
   local self_is_thinking = self._state == THINKING or self._state == STARTING_THINKING

   for i = units_start,units_start + (num_units - 1) do
      local u = units[i]

      local should_think = self_is_thinking or not self._active_unit or self:_is_strictly_better_than_active(u.unit)
      if should_think then
         -- If this frame is running/getting ready to run, only think units with > priority than currently active.
         self._log:spam('slow start thinking on %s', u.unit:get_name())
         u.unit:_start_thinking(local_args, u.state)
         self:_record_slow_think(timeout, u.unit)
      end
   end
end

-- Returns true iff a child unit is active (and is different from the previously active unit)
function ExecutionFrame:_restart_thinking(entity_state, debug_reason)
   self._log:detail('_restart_thinking (reason:%s, state:%s)', debug_reason, self._state)

   if self._state == 'running' then
      -- if we're running, we need to thunk over to the ai thread in order to restart thinking
      local calling_thread = stonehearth.threads:get_current_thread()
      assert(calling_thread == self._thread, 'on wrong thread in execution frame restart thinking')
   end

   if not (self._state == 'thinking' or self._state == 'starting_thinking' or self._state == 'ready' or self._state == 'running') then
      self._log:spam('_restart_thinking returning without doing anything.(state:%s)', self._state)
      return false
   end

   local state_to_set = entity_state
   if not state_to_set then
      self._log:debug('_capture_entity_state')
      state_to_set = self:_create_entity_state()
   end
   self:_set_current_entity_state(state_to_set)


   -- if any of our filters don't want to run this sub-tree, just bail.
   for _, unit in pairs(self._execution_filters) do
      if not unit:_should_start_thinking(self._args, entity_state) then
         return false
      end
   end
   
   -- see which units we can restart and clone the state for them

   local rethinking_units = {}
   if self._rerun_unit and self._rerun_unit:get_state() ~= DEAD and self:_is_strictly_better_than_active(self._rerun_unit) then
      self._log:spam('adding rerun unit %s', self._rerun_unit:get_name())      
      rethinking_units[self._rerun_unit] = self:_clone_entity_state('new speculation for unit')
   else
      self._log:spam('no rerun unit found')
   end

   -- If we have a rerun unit, take all the units that are strictly better than it to rethink.
   -- Anbody not rethinking right now gets added to the slow_rethink_units.
   local slow_rethink_units = {}
   for _, unit in pairs(self._execution_units) do
      if self:_is_strictly_better_than_active(unit) then
         local new_state = self:_clone_entity_state('new speculation for unit')
         local think_now = true

         -- figure out whether this unit should think now or later.
         if SLOW_START_ENABLED then
            if self._rerun_unit and self._rerun_unit ~= unit then
               -- realtime actions get to start right away
               think_now = unit:get_action().realtime  
               if not think_now then
               -- Higher-priority units than our re-run unit MUST be allowed to run ASAP.  Otherwise,
               -- we can easily starve important tasks from running.  Likewise, realtime tasks must be allowed to
               -- think, otherwise reacting to real-time events (e.g. combat) becomes borked.
                  think_now = self._rerun_unit:get_priority() < unit:get_priority()
               end
            end
         end

         -- add the unit to the `rethinking_units` or `slow_rethink_units` list
         if think_now then
            self._log:spam('fast start unit %s', unit:get_name())
            rethinking_units[unit] = new_state
         else
            self._log:spam('slow start unit %s', unit:get_name())
            table.insert(slow_rethink_units, { unit = unit, state = new_state })
         end
      end
   end

   self:_record_fast_think(rethinking_units)

   -- Sort the slow_rethink_units, first by priority, then by number of times run.  This will
   -- ensure that higher-priority slow_rethink_units get to think first.
   table.sort(slow_rethink_units, function(l, r)
         if l.unit:get_priority() > r.unit:get_priority() then
            return true
         elseif l.unit:get_priority() < r.unit:get_priority() then
            return false
         end
         return l.unit._num_runs > r.unit._num_runs
      end)


   -- Launch all our slow starts.  SLOWSTART_MAX_UNITS defines how many units per callback we should
   -- process, while SLOWSTART_TIMEOUTS defines the wait times for each callback.
   self:_destroy_slow_timers()
   if #slow_rethink_units > 0 then
      local local_args = self._args
      local num_left = #slow_rethink_units

      local i = 1
      local units_start = 1
      while num_left > 0 do
         local num_units_used = SLOWSTART_MAX_UNITS[i]
         if num_units_used + units_start - 1 > #slow_rethink_units then
            num_units_used = #slow_rethink_units - units_start + 1
         end

         self._log:spam('deferring %s units for %s ms', num_units_used, SLOWSTART_TIMEOUTS[i])

         -- Sigh, don't bind to the variable outside the lexical body....
         local local_units_start = units_start
         local timeout = SLOWSTART_TIMEOUTS[i]
         local new_timer = radiant.set_realtime_timer(SLOWSTART_TIMEOUTS[i], function()
               self:_do_slow_thinking(local_args, slow_rethink_units, local_units_start, num_units_used, timeout)
            end)
         table.insert(self._slow_start_timers, new_timer)

         num_left = num_left - num_units_used
         units_start = units_start + num_units_used
         i = i + 1
      end
   end

   -- Continue to process our currently rethinking units.
   local entity_location
   if self._log:is_enabled(radiant.log.DETAIL) then
      entity_location = radiant.entities.get_world_grid_location(self._entity)
   end

   local current_state = self._state  
   local current_active_unit = self._active_unit

   for unit, entity_state in pairs(rethinking_units) do
      self._log:detail('calling start_thinking on unit "%s" (u:%d state:%s ai.CURRENT.location:%s actual_location:%s).',
                        unit:get_name(), unit:get_id(), unit:get_state(), entity_state.location, entity_location)
      assert(unit._state == 'stopped' or unit._state == 'thinking' or unit._state == 'ready')
      unit:_start_thinking(self._args, entity_state)

      -- if units bail or abort, the current pcall should have been interrupted.
      -- verify that this is so.  The only valid change should be to a ready state.

      if self._state ~= current_state then
         assert(self._state == READY, string.format('state changed in _restart_thinking %s -> %s', current_state, self._state))
      end
   end

   if self._active_unit ~= current_active_unit then
      self._log:spam('active unit changed from %s to %s in _restart_thinking.  skipping election...',
                     current_active_unit and current_active_unit:get_name() or '-none-',
                     self._active_unit and self._active_unit:get_name() or '-none-')
      return false
   end
   
   self._log:spam('choosing best execution unit of candidates...')

   -- if anything became ready during the think, use it immediately.
   local unit = self:_get_best_execution_unit()
   if unit and unit ~= self._active_unit then
      self._log:detail('%s -> %s', self._active_unit and self._active_unit:get_name() or '', unit:get_name())
      local think_output = self._saved_think_output[unit]
      assert(think_output)

      if self._active_unit then
         self._log:detail('stopping old active unit "%s" in _restart_thinking.', self._active_unit:get_name())
         self._active_unit:_stop()
         self._active_unit = nil
      end
      self._log:detail('using "%s" as active unit in _restart_thinking.', unit:get_name())
      self:_set_active_unit(unit, think_output)
      self:_set_state(READY)
      return true
   end
   return false
end

function ExecutionFrame:_start_thinking_from_stopped(args, entity_state)   
   self._log:debug('_start_thinking_from_stopped')
   
   self._args = args
   if self._debug_info then
      self._debug_info:modify(function (o)
            o.args = stonehearth.ai:format_args(self._args)
         end)
   end

   self._log:debug('set execution frame arguments to %s %s', self._activity_name, stonehearth.ai.format_args(self._args))

   -- errata note 1 : this is actually probably correct, but look at it?  wtf?  this code is extremely
   -- opaque.
   self:_set_state(STARTING_THINKING)
   if not self:_restart_thinking(entity_state, "thinking from stopped") then
      self:_set_state(THINKING)
   end
end

function ExecutionFrame:_do_destroy()
   if self._position_trace then
      self._position_trace:destroy()
      self._position_trace = nil
   end
   self._entity:get_component('stonehearth:ai')
                  :_unregister_execution_frame(self._activity_name, self)

   if self._carry_listener then
      self._carry_listener:destroy()
      self._carry_listener = nil
   end
   if self._debug_info then
      self._debug_info:destroy()
      self._debug_info = nil
   end
end

function ExecutionFrame:start_thinking(args, entity_state)
   if self._aborting then
      self._log:debug('clearing abort status from previous (presumedly failed) execution.')
      self._aborting = false
   end
   
   self._log:spam('start_thinking (state: %s)', self._state)
   assert(args, "start_thinking called with no arguments")
   assert(self:get_state() == STOPPED, string.format('start thinking called from non-stopped state "%s"', self:get_state()))

   self:_start_thinking(args, entity_state)
   if self._state == READY then
      assert(self._active_unit_think_output)
      return self._active_unit_think_output
   end
   self._log:spam('not immediately ready after start_thinking.')
end

function ExecutionFrame:set_think_progress_cb(think_progress_cb)
   self._log:spam('registering think progress callback (state: %s)', self._state)
   if not think_progress_cb then
      self._think_progress_cb = nil
      return
   end

   local calling_thread = stonehearth.threads:get_current_thread()
   self._think_progress_cb = function(msg, arg)
         if calling_thread and not calling_thread:is_running() then
            -- this interrupt is synchronous to make sure we deliver the callback in the current.
            -- state.  for example, soon after we return from this function, a task might kill
            -- an action that's running this frame (ending up stopping the frame before the 
            -- thread which wants the notification actually gets scheduled!)
            self._log:detail('ai thread is not running trying to deliver think progress (%s) notification.  thunking.', msg)
            calling_thread:interrupt(function()
                  assert(self._think_progress_cb)
                  self._log:detail('calling think progress (%s) notification with thunk', msg)
                  think_progress_cb(self, msg, arg)
               end)
         else
            -- sometimes there is no thread (e.g. in pathfinder callbacks).
            -- just use whatever thread we're on.
            self._log:detail('calling think progress (%s) notification without thunk', msg)
            think_progress_cb(self, msg, arg)
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
         self:start_thinking(args)
         self:wait_until(READY)
      end
      if self:in_state(READY) then
         self:_start()
         self:wait_until(STARTED)
      end
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
         radiant.log.return_logger(self._log)
         self._log = nil
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
   self:_destroy_slow_timers()
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_thinking_from_ready()
   self._log:debug('_stop_thinking_from_ready (state: %s)', self._state)

   for _, unit in pairs(self._execution_units) do
      unit:_stop_thinking()
   end
   self:_set_active_unit(nil)
   self:_destroy_slow_timers()
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_thinking_from_started()
   self._log:debug('_stop_thinking_from_started')
   assert(self._active_unit)
   
   for _, unit in pairs(self._execution_units) do
      if unit == self._active_unit then
         -- nothing to do!
      elseif not self:_is_strictly_better_than_active(unit) then
         self._log:debug('%s is running, so calling stop_thinking on %s.', self._active_unit:get_name(), unit:get_name())
         unit:_stop_thinking() -- this guy has no prayer...  just stop
      else
         self._log:debug('letting %s continue thinking while %s is running.', unit:get_name(), self._active_unit:get_name())
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
end

function ExecutionFrame:_unit_ready_from_starting_thinking(unit, think_output)
   self:_do_unit_ready_bookkeeping(unit, think_output)
end

function ExecutionFrame:_unit_halted_from_starting_thinking(unit, think_output)
   -- halted units will stop themselves.  if everything is stopped, propogate the halt
   -- upstream
   for _, unit in pairs(self._execution_units) do
      local name = unit:get_name()
      local state = unit:get_state()

      self._log:spam('  unit %s -> (state:%s)', name, state)
      if state ~= HALTED then
         return
      end
   end
   self._log:detail('all units halted.  halting execution frame.')
   if self._think_progress_cb then
      self._log:debug('sending halted notification')
      self._think_progress_cb('halted')
   end
   -- now the frame is a zombie.  boo hoo =(
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
      if self._think_progress_cb then
         self._log:debug('sending unready notification')
         self._think_progress_cb('unready')
      end
   end
end

function ExecutionFrame:_set_active_unit(unit, think_output)
   -- seems like a REALLY good thing to do, put probably lots of fallout
   -- assert(not unit or not self._active_unit, 'cannot change active unit without disposing of the previous one')

   local aname = self._active_unit and self._active_unit:get_name() or 'none'
   local uname = unit and unit:get_name() or 'none'
   self._log:detail('replacing active unit "%s" with "%s"', aname, uname)
   
   self._active_unit = unit
   self._active_unit_cost = unit and unit:get_cost() or 0
   self._active_unit_think_output = think_output

   if self._current_entity_state then
      local new_entity_state
      if self._active_unit then
         new_entity_state = unit:get_current_entity_state()
         self._log:spam('copying unit state %s to current state %s', tostring(new_entity_state), tostring(self._current_entity_state))
      else
         new_entity_state = self._saved_entity_state
         self._log:spam('copying saved state %s to current state %s', tostring(new_entity_state), tostring(self._current_entity_state))
      end

      -- clear the table to make sure nil values are copied from the new to the current state
      copy_entity_state(self._current_entity_state, new_entity_state)
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

   -- Ensure all lower priority units are stopped, so they don't waste our time.
   self:_stop_thinking_from_started()

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

   self._rerun_unit = self._active_unit
   self._active_unit = nil
   self:_set_state(FINISHED)
end

function ExecutionFrame:_stop_from_started()
   assert(self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:_stop()
   end
   self:_set_active_unit(nil)
   self:_destroy_slow_timers()
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_from_thinking()
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:_stop_thinking()
   end
   self:_destroy_slow_timers()
   self:_set_state(STOPPED)
end

function ExecutionFrame:_stop_from_running()
   -- stop from running can only happen if there are no other threads running.
   -- this can occur when an entity is destroyed from a C callback into the
   -- game engine.  if we were running at the time, we'll take the standard
   -- unwind pcall path where we simply get destroyed
   assert(self:_no_other_thread_is_running())
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
   self:_destroy_slow_timers()
   self:_set_state(STOPPED)
end
   
function ExecutionFrame:_stop_from_finished()
   self._log:detail('_stop_from_finished')
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      self._log:detail('stopping execution unit "%s"', unit:get_name())
      unit:_stop(true)
   end
   self:_destroy_slow_timers()
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
   self._execution_filters = {}
   self._execution_units = {}
   self:_set_state(DEAD)
   self:_do_destroy()
end

function ExecutionFrame:_destroy_from_stopped()
   self._log:debug('_destroy_from_stopped (state: %s)', self._state)
   assert(not self._active_unit)
   for _, unit in pairs(self._execution_units) do
      unit:_destroy()
   end
   self._execution_filters = {}
   self._execution_units = {}
   self:_set_state(DEAD)
   self:_do_destroy()
end

function ExecutionFrame:_destroy_from_running()
   -- verify either we're the thread running or we've been destroyed from a C callback
   assert(self:_no_other_thread_is_running())
   
   for _, unit in pairs(self._execution_units) do
      unit:_destroy()
   end
   self._execution_filters = {}
   self._execution_units = {}
   self:_set_state(DEAD)
   self:_do_destroy()
end

function ExecutionFrame:abort()
   self._log:info('abort')
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
   -- It is possible this function can be called from within a destroyed frame (whose logger has gone
   -- back to the pool).  Hence, the need for a logging alternate.
   if self._log then
      self._log:detail(msg)
   else
      radiant.log.spam('ai_ef', msg)
   end
   local runstack = self._thread:get_thread_data('stonehearth:run_stack')
   for i = #runstack,1,-1 do
      local f = runstack[i]
      if self._log then
         self._log:spam('  [%d] - (%5d) %s', i, f._id, f._activity_name, stonehearth.ai.format_args(f._args))
      else
         radiant.log.spam('ai_ef', '  [%d] - (%5d) %s', i, f._id, f._activity_name, stonehearth.ai.format_args(f._args))
      end
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

   -- errata note 1 : el oh el.  wtf... this isn't going to do anything from the RUNNING
   -- state, which means none of our peers ACTUALLY get start_thinking called on them.
   -- GOD.
   self:_restart_thinking(nil, "unwinding stack")
end
 
function ExecutionFrame:_add_action_from_thinking(key, entry)
   local unit = self:_add_execution_unit(key, entry)
   local current_state = self:_clone_entity_state('new speculation for unit')
   unit:_start_thinking(self._args, current_state)
end

function ExecutionFrame:_add_action_from_ready(key, entry)
   self:_add_execution_unit(key, entry)
   -- we could kick it in the pants now, but let's not bother...
end

function ExecutionFrame:_add_action_from_running(key, entry)
   local unit = self:_add_execution_unit(key, entry)
   if self:_is_strictly_better_than_active(unit) then   
      self._log:detail('new action %s is better than active %s.  calling start_thinking', unit:get_name(), self._active_unit:get_name())
      unit:_start_thinking(self._args, self:_create_entity_state())
   else
      self._log:detail('new action %s is not better than active %s.', unit:get_name(), self._active_unit:get_name())
   end
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
      if self._think_progress_cb then
         self._log:debug('sending unready notification')
         self._think_progress_cb('unready')
      end
      -- errata note 1 : again, this one is verified correct, but gross and offends
      -- my programmer sensibilities.
      self:_set_state(STARTING_THINKING)
      if not self:_restart_thinking(self._current_entity_state, "removing action") then
         self:_set_state(THINKING)
      end
   else
      self:_remove_execution_unit(unit)
   end
end

function ExecutionFrame:_remove_action_from_running(unit)
   self._log:detail('_remove_action_from_running (state: %s)', self._state)
   if unit == self._active_unit then
      if not self._aborting then
         self._thread:interrupt(function()
            self:_stop()
            self:_set_active_unit(nil)
            self:_remove_execution_unit(unit)
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
         self._saved_think_output[u] = nil
         self:_update_debug_info()
         return
      end
   end

   -- Review Comment: Hey Tony, We need to remove the execution filters, too, right? 
   -- Is there a reason they are stored in 2 different structures? (--sdee, chris)
   for key, u in pairs(self._execution_filters) do
      if unit == u then
         self._log:debug('removing execution unit "%s"', unit:get_name())
         unit:_destroy()
         self._execution_filters[key] = nil
         self._saved_think_output[u] = nil
         return
      end
   end
end

function ExecutionFrame:on_action_index_changed(add_remove, key, entry)
   -- injecting or remove and action can cause the current execution stack to be
   -- reevaluated (e.g. injecting the first stonehearth:work action on an idle worker).
   -- make sure we do this on the ai thread!
   self._thread:interrupt(function()
         if self._log:is_enabled(radiant.log.DETAIL) then
            local key_name
            if type(key) == 'table' and key.name then
               key_name = 'dynamic_action:' .. key.name
            else
               key_name = tostring(key_name)
            end
            self._log:debug('on_action_index_changed %s (key:%s state:%s)', add_remove, key_name, self._state)
         end
         if self:get_state() ~= DEAD then
            local unit = self._execution_units[key]
            if not unit then 
               unit = self._execution_filters[key]
            end
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
      end)
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
   if action.type == 'filter' then
      assert(not self._execution_filters[key])
      self._execution_filters[key] = unit
   else
      assert(not self._execution_units[key])
      self._execution_units[key] = unit
      self:_update_debug_info()
   end
   return unit
end

function ExecutionFrame:_set_current_entity_state(state)
   self._log:debug('set_current_entity_state')
   assert(state)

   self:_spam_entity_state(state, 'set_current_entity_state')

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
   local cloned = create_entity_state()

   cloned.location = s.location and Point3(s.location.x, s.location.y, s.location.z)
   cloned.carrying = s.carrying
   cloned.future = s.future

   self:_spam_entity_state(cloned, 'cloning current state %s to %s %s', self._current_entity_state, name, cloned)

   return cloned
end

function ExecutionFrame:_create_entity_state()
   local state = create_entity_state()
   state.carrying = radiant.entities.get_carrying(self._entity)
   state.location = radiant.entities.get_world_grid_location(self._entity)
   state.future = false

   self:_spam_entity_state(state, 'capturing current entity state')
   return state
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
   end
   self._log:spam('  unit %s priority %d <= active unit "%s" priority %d.  therefore is not better!',
                  unit:get_name(), unit_priority, self._active_unit:get_name(), active_priority)
   return false
end

function ExecutionFrame:_get_best_execution_unit()
   local best_priority, best_cost = 0, INFINITE
   local active_units = nil
   
   self._log:spam('%s looking for best execution unit for "%s"', self._entity, self._activity_name)
   for _, unit in pairs(self._execution_units) do
      local name = unit:get_name()
      local cost = unit:get_cost()
      local priority = unit:get_priority()
      local is_runnable = unit:is_runnable()

      self._log:spam('  unit %s -> (priority:%d cost:%.3f weight:%d runnable:%s state:%s)',
                     name, priority, cost, unit:get_weight(), is_runnable, unit:get_state())

      if is_runnable then
         local replace_existing = priority > best_priority or (priority == best_priority and cost < best_cost)
         local add_candidate = replace_existing or (priority == best_priority and cost == best_cost)

         if replace_existing then
            self._log:spam('    way better!  wiping out old candidates.')
            -- a new bar has been set, wipe out the old list of candidate actions
            active_units = {}
            best_priority, best_cost = priority, cost
         end
         if add_candidate then
            self._log:spam('    adding to candidates.')
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
   local total_candidates = #active_units
   local active_unit = active_units[math.random(total_candidates)]
   self._log:spam('%s  best unit for "%s" is "%s" (priority:%d  cost:%.3f  total_candidates:%d)',
                   self._entity, self._activity_name, active_unit:get_name(), best_priority, best_cost, total_candidates)

   return active_unit
end

function ExecutionFrame:get_debug_info(depth)
   if not self._debug_info then
      self._debug_info = radiant.create_datastore({ depth = depth + 1 })
      self:_update_debug_info()
   end
   return self._debug_info
end

function ExecutionFrame:_update_debug_info()
   if self._debug_info then
      self._debug_info:modify(function (o)
         o.id = 'f:' .. tostring(self._id)
         o.state = self._state
         o.activity = {
            name = self._activity_name,
         }
         o.args = stonehearth.ai:format_args(self._args)
         o.execution_units = {
            n = 0,
         }
         for _, unit in pairs(self._execution_units) do
            table.insert(o.execution_units, unit:get_debug_info(o.depth))
         end
         table.sort(o.execution_units,
            function (a_datastore, b_datastore)
               local a, b = a_datastore:get_data(), b_datastore:get_data()
               local a_rank = debug_info_state_order[a.state]
               local b_rank = debug_info_state_order[b.state]
               if a_rank ~= b_rank then
                  return a_rank < b_rank
               end
               return a.name < b.name
            end
         )
      end)
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
   self._log:debug('state change %s -> %s', self._state, state)
   self._state = state
   
   if self._debug_info then
      self._debug_info:modify(function (o)
            o.state = self._state
         end)
   end
   local going_to_frame = self._thread:get_thread_data('stonehearth:unwind_to_frame')
   assert(not going_to_frame)
 
   if self._state == self._waiting_until then
      self._calling_thread:resume('transitioned to ' .. self._state)
   end
   if self._think_progress_cb and state == READY then
      assert(self._active_unit)
      assert(self._active_unit_think_output)
      self._think_progress_cb('ready', self._active_unit_think_output)
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
         radiant.log.debug('ai_exec', 'finished unwinding!')
      elseif err == UNWIND_NEXT_FRAME then
         self:_cleanup_protected_call_exit()
         exit_handler()
         self:_log_stack('in UNWIND_NEXT_FRAME handler...')         
         -- it's magic.  don't question it (there's a 2nd pcall in run...
         self:_exit_protected_call(UNWIND_NEXT_FRAME_2)
      else
         radiant.log.info('ai_exec', "aborting on error '%s' (state:%s)", err, self._state)
         -- If the action is removed from the running state, we might be dead, and we can't
         -- transition from dead to stopped.
         if self._state ~= DEAD then
            self:_stop(true)         
            assert(self._state == STOPPED, string.format('stop during abort took us to non-stopped state "%s"', self._state))
         end
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
   if self._thread:is_running() then
      return true
   end
   local current = stonehearth.threads:get_current_thread()
   if not current then
      return true
   end
   if current:get_thread_data('autotest_framework:is_autotest_thread') then
      return true
   end
   return false
end

function ExecutionFrame:_spam_entity_state(state, format, ...)
   if self._log:is_enabled(radiant.log.SPAM) then
      self._log:spam(format, ...)
      for key, value in pairs(state) do      
         self._log:spam('  CURRENT.%s = %s', key, value)
      end   
   end
end

function ExecutionFrame:get_cost()
   return self._active_unit and self._active_unit_cost or 0
end

return ExecutionFrame
