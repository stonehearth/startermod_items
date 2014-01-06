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
      unit:initialize(args)
      radiant.events.listen(unit, 'ready', self, self.on_unit_state_change)
      radiant.events.listen(unit, 'priority_changed', self, self.on_unit_state_change)
   end
end

function ExecutionFrame:terminate()
   for uri, unit in pairs(self._execution_units) do
      unit:destroy()
   end
   self._execution_units = {}

   self:_set_state(DEAD)
   radiant.events.unpublish(self)
end

function ExecutionFrame:get_activity_name()
   return self._activity_name
end

function ExecutionFrame:get_active_execution_unit()
   return self._active_unit
end

-- xxx
function ExecutionFrame:on_unit_state_change(unit)
   if self._in_start_processing then
      return
   end
   
   if self:_is_better_execution_unit(unit) then
      -- uh oh!  now what?
      if self._state == PROCESSING then
         -- no sweat.  just start running
         self._active_unit = unit
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
         unit:start_background_processing()
      end
   end
   self._in_start_processing = false
   
   self._active_unit = self:_get_best_execution_unit()
   if self._active_unit then
      self:_set_state(READY)
   end
end

function ExecutionFrame:start()
   assert(self._state == READY)
   assert(self._active_unit)
   self:_set_state(STARTING)
   for uri, unit in pairs(self._execution_units) do
      if unit == self._active_unit then
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         if unit:get_version() == 1 then
            local name = unit:get_name()
            assert(false, 'port all version1 actions please!')
         end
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX
         -- XXXXXXXXXXXXXXXXXXXXXX

         unit:start()
      elseif not self:_is_better_execution_unit(unit) then
         -- this guy has no prayer...  just stop
         unit:stop_background_processing()
      else
         -- let better ones keep processing.  they may prempt in the future
      end
   end
end

function ExecutionFrame:_run()
   assert(self._state == STARTING)
   self:_set_state(RUNNING)
   self._active_unit:run()
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
      self._active_unit:stop()
      self._active_unit:start_background_processing()
      self._state = CONSTRUCTED
   end
end

function ExecutionFrame:stop()
   self:_set_state(STOPPING)
   for uri, unit in pairs(self._execution_units) do
      unit:stop()
   end
end

function ExecutionFrame:suspend_until_ready()
   if not self._active_unit then
      log:info('suspending thread until ready to go')
      radiant.events.listen(self, 'ready', function()
         self._ai:resume_thread()
         return radiant.events.UNLISTEN
      end)
      self._ai:suspend_thread()
      log:info('ready! thread resumed')
   end
   assert(self._active_unit)
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
   
   log:spam('%s looking for best execution unit for %s', self._entity, self._activity_name)
   for uri, unit in pairs(self._execution_units) do
      if unit:is_runnable() then
         local name = unit:get_name()
         local priority = unit:get_priority()
         log:spam('  unit %s has priority %d', name, priority)

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
      log:debug('no unit ready to run yet.  chillin.')
      return nil
   end
   
   -- choose a random unit amoung all the units with the highest priority (they all tie)
   local active_unit = active_units[math.random(#active_units)]
   log:spam('%s  best unit for %s is %s (priority: %d)', self._entity, self._activity_name, active_unit:get_name(), best_priority)
   
   return active_unit
end

function ExecutionFrame:_set_state(state)
   assert (self._state ~= state)
   self._state = state
   radiant.events.trigger(self, self._state, self, self._active_unit)
   radiant.events.trigger(self, 'state_changed', self, self._active_unit)
end

return ExecutionFrame
