local WorkerDispatcher = class()
local DISPATCHER_WAIT_TIME = require('constants').worker_scheduler.DISPATCHER_WAIT_TIME

--- Create a new worker dispatcher
-- @param worker The worker who will receive new tasks
-- @param dispatch_fn The function to call when there's a new task to do
function WorkerDispatcher:__init(worker, dispatch_fn)
   self._worker = worker
   self._dispatch_fn = dispatch_fn
   self._solutions = {}
end

--- Returns the worker assigned to this dispatcher
function WorkerDispatcher:get_worker()
   return self._worker
end

--- Add a solution to the dispatcher
-- After adding the first solution, the dispatcher will wait DISPATCHER_WAIT_TIME to
-- see if a better one comes in.  When the timer expires, it chooses the task with
-- the highest priority and dispatches it to the worker
-- @param priority The priority of this solution
-- @param action A table containing the action to execute and all its arguments
function WorkerDispatcher:add_solution(priority, action)
   -- If we haven't started the timer yet, go ahead and do so.
   if #self._solutions == 0 then
      self._countdown = DISPATCHER_WAIT_TIME
      radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._check_dispatch)
   end

   local solution = {
      priority = priority,
      action = action,
   }  
   table.insert(self._solutions, solution)
end

--- Destroy the worker dispatcher
-- The dispatcher should get destroyed when the worker is busy.  If there are pending
-- solutions, this will make sure they don't get sent (see add_solution)
function WorkerDispatcher:destroy()
   self:_reset()
end

--- Reset the dispatcher to it's initialized state.
-- Kill the timer, clear the solution table, etc.
function WorkerDispatcher:_reset()
   if self._countdown then
      radiant.events.unlisten(radiant.events, 'stonehearth:gameloop', self, self._check_dispatch)
      self._countdown = nil
   end
   self._solutions = {}
end

--- Check to see if it's time to dispatch a solution
-- If it's not time to dispatch a solution yet, decrement the timer.  If it is time,
-- unlisten to the gameloop event and dispatch!
function WorkerDispatcher:_check_dispatch()
   assert(self._countdown, '_check_dispatch called after countdown has expired')
   assert(#self._solutions, '_check_dispatch called with no solutions available')
   
   self._countdown = self._countdown - 1
   if self._countdown == 0 then
      local best = self:_get_best_solution()
      self:_reset()
      self._dispatch_fn(best.priority, best.action)
   end
end

--- Returns the solution with the highest priority
function WorkerDispatcher:_get_best_solution()
   local best
   for _, solution in ipairs(self._solutions) do
      if not best or solution.priority > best.priority then
         best = solution
      end
   end
   return best
end

return WorkerDispatcher
