
local ExecutionFrame = class()
local log = radiant.log.create_logger('ai.component')

local THINKING = 0
local WAITING = 1
local RUNNING = 2

function ExecutionFrame:__init(ai, execution_units, activity)
   self._ai = ai
   self._entity = self._ai:get_entity()
   self._activity_name = activity[1]   
   self._execution_units = execution_units
   
   local args = { select(2, unpack(activity)) }
   for uri, unit in pairs(self._execution_units) do
      unit:join_execution_frame(self, args)
   end
end

function ExecutionFrame:destroy()
   for uri, unit in pairs(self._execution_units) do
      unit:leave_execution_frame(self)
   end
   self._execution_units = {}
end

function ExecutionFrame:get_activity_name()
   return self._activity_name
end

function ExecutionFrame:execute()
   self._current_unit = self:_get_next_execution_unit()
   
   assert(self._current_unit) -- this is totally valid... just need to figure out what to do here...
   if self._current_unit then      
      return self._current_unit:execute_frame(self)
   end
end

function ExecutionFrame:on_action_priority_changed(unit)
   if not self._current_unit then
      -- still initializing the think
      return
   end
   
   local abort = false
   local new_priority = unit:get_priority()
   if unit == self._current_unit then
      -- if the current unit is trying to change its priority, we need to check the
      -- current frame if that might cause some other unit to be run.  yes, this is
      -- a weird thing for an unit to do, but it's easy to be robust here.
      local best_unit = self:_get_next_execution_unit()
      abort = unit ~= best_unit
   else
      -- if another action is elevating it's priority above the current action
      -- abort.
      abort = new_priority > self._current_unit:get_priority()
   end

   if abort then
      self._ai:abort(string.format('unit priority change resulted in new best unit'))
   end
end

function ExecutionFrame:get_current_unit()
   return self._current_unit
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

function ExecutionFrame:_get_next_execution_unit()  
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

return ExecutionFrame
