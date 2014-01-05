local ExecutionFrame = class()
local log = radiant.log.create_logger('ai.execution_frame')

-- these values are in the interface, since we trigger them as events.  don't change them!
local CONSTRUCTED = 'constructed'
local PROCESSING = 'processing'
local READY = 'ready'
local STARTING = 'starting'
local RUNNING = 'running'
local STOPPING = 'stopping'
local DEAD = 'dead'

function ExecutionFrame:__init(ai, execution_units, activity, complete_fn)
   self._ai = ai
   self._entity = self._ai:get_entity()
   self._activity_name = activity[1]   
   self._execution_units = execution_units
   self._complete_fn = complete_fn
   self._state = CONSTRUCTED
   
   -- notify everyone that they party's started
   local args = { select(2, unpack(activity)) }
   for uri, unit in pairs(self._execution_units) do
      unit:join_execution_frame(self, args)
   end
end

function ExecutionFrame:_destroy()
   if self._state == RUNNING then
      self:stop()
   end
   
   for uri, unit in pairs(self._execution_units) do
      unit:leave_execution_frame(self)
      unit:_destroy()
   end
   self._execution_units = {}
   
   if self._complete_fn then
      self._complete_fn()
   end
   self:_set_state(DEAD)
   radiant.events.unpublish(self)
end

function ExecutionFrame:get_activity_name()
   return self._activity_name
end

function ExecutionFrame:get_active_execution_unit()
   return self._best_unit
end

function ExecutionFrame:on_unit_state_change(unit)
   if self._in_start_processing then
      return
   end
   
   if self:_is_better_execution_unit(unit) then
      -- uh oh!  now what?
      if self._state == PROCESSING then
         -- no sweat.  just start running
         self._best_unit = unit
         self:_set_state(READY)
         return
      end
      
      -- fuck...
      assert(false)
   end
end

function ExecutionFrame:start_background_processing()
   assert(self._state == CONSTRUCTED)
   
   self._in_start_processing = true
   self:_set_state(PROCESSING)
   for uri, unit in pairs(self._execution_units) do
      if self:_is_better_execution_unit(unit) then
         unit:start_background_processing(self)
      end
   end
   self._in_start_processing = false
   
   self._best_unit = self:_get_best_execution_unit()
   if self._best_unit then
      self:_set_state(READY)
   end
end

function ExecutionFrame:start()
   assert(self._state == READY)
   assert(self._best_unit)
   self:_set_state(STARTING)
   for uri, unit in pairs(self._execution_units) do
      if unit == self._best_unit then
         unit:start_frame(self)
      elseif not self:_is_better_execution_unit(unit) then
         -- this guy has no prayer...  just stop
         unit:stop_background_processing(self)
      else
         -- let better ones keep processing.  they may prempt in the future
      end
   end
end

function ExecutionFrame:_run()
   assert(self._state == STARTING)
   self:_set_state(RUNNING)
   self._best_unit:run_frame(self)
end

function ExecutionFrame:run()
   if self._state == CONSTRUCTED then
      self:start_background_processing()
   end
   if self._state == PROCESSING then
      self:suspend_until_ready()   
   end
   if self._state == READY then
      self:start()
   end
   if self._state == STARTING then
      self:_run()
      return
   end
   -- explicity do not call stop.  the caller must
   -- do this, or it will be done automatically in destroy.
   assert(false, string.format('invalid state %s in run', self._state))
end

function ExecutionFrame:loop()
   while true do
      self:run()
      self._best_unit:stop_frame(self)
      self._best_unit:start_background_processing(self)
      self._state = CONSTRUCTED
   end
end

function ExecutionFrame:stop()
   self:_set_state(STOPPING)
   for uri, unit in pairs(self._execution_units) do
      unit:stop_frame(self)
   end
end

function ExecutionFrame:suspend_until_ready()
   if not self._best_unit then
      log:info('suspending thread until ready to go')
      radiant.events.listen(self, 'ready', function()
         self._ai:resume_thread()
         return radiant.events.UNLISTEN
      end)
      self._ai:suspend_thread()
      log:info('ready! thread resumed')
   end
   assert(self._best_unit)
end

function ExecutionFrame:format_args(args)
   local msg = ''
   if args then
      for _, arg in ipairs(args) do
         if #msg > 0 then
            msg = msg .. ', '
         end
         msg = msg .. tostring(arg)
      end
   end
   return msg
end

-- *strictly* better.  no run-offs for latecomers.
function ExecutionFrame:_is_better_execution_unit(unit)
   if not self._best_unit then
      return true
   end
   if unit == self._best_unit then
      return false
   end
   if unit:get_priority() > self._best_unit:get_priority() then
      return true
   end
   return false
end

function ExecutionFrame:_get_best_execution_unit()
   local best_priority = 0
   local best_units = nil
   
   log:spam('%s looking for best execution unit for %s', self._entity, self._activity_name)
   for uri, unit in pairs(self._execution_units) do
      if unit:is_runnable(self) then
         local name = unit:get_name()
         local priority = unit:get_priority()
         log:spam('  unit %s has priority %d', name, priority)

         if priority >= best_priority then
            if priority > best_priority then
               -- new best_priority found, wipe out the old list of candidate actions
               best_units = {}
            end
            best_priority = priority
            for i = 1, unit:get_weight() do
               table.insert(best_units, unit)
            end
         end
      end
   end

   if not best_units then
      log:debug('no unit ready to run yet.  chillin.')
      return nil
   end
   
   -- choose a random unit amoung all the units with the highest priority (they all tie)
   local best_unit = best_units[math.random(#best_units)]
   log:spam('%s  best unit for %s is %s (priority: %d)', self._entity, self._activity_name, best_unit:get_name(), best_priority)
   
   return best_unit
end

function ExecutionFrame:_set_state(state)
   assert (self._state ~= state)
   self._state = state
   radiant.events.trigger(self, self._state, self, self._best_unit)
   radiant.events.trigger(self, 'state_changed', self, self._best_unit)
end

return ExecutionFrame
