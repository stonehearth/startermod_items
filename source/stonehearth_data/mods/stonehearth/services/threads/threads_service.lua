local ThreadsService = class()
local log = radiant.log.create_logger('threads')

function ThreadsService:__init()
   -- SUSPEND_THREAD is a unique, non-integer token which indicates the thread
   -- should suspend.  It must be non-intenger, as yielding an int means "wait
   -- until this time".  By creating a table, we guarantee the value of
   -- SUSPEND_THREAD is unique when compared with ==.  The name is for
   -- cosmetic purposes and aids in debugging.
   self.SUSPEND_THREAD = { name = "SUSPEND_THREAD" }
   self.KILL_THREAD = { name = "KILL_THREAD" }

   self._next_thread_id = 1
   self._threads = {}
   self._scheduled = {}
   self._waiting_until = {}
   
   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._on_event_loop)
end

function ThreadsService:_on_event_loop(e)
   local now = e.now

   for handle, pred in pairs(self._waiting_until) do
      local run = false
      if type(pred) == 'function' then
         run = pred(now)
      elseif type(pred) == 'number' then
         run = radiant.gamestate.now() >= pred
      elseif pred == self.SUSPEND_THREAD then
         run = false
      else
         assert(false)
      end
      if run then
         self:schedule_thread(handle)
      end
   end

   local dead = {}
   for handle, co in pairs(self._scheduled) do
      -- run it
      self._running_thread = co
      local success, wait_obj = coroutine.resume(co)
      self._running_thread = nil
      
      if not success then
         radiant.check.report_thread_error(co, 'thread error: ' .. tostring(wait_obj))
         self._scheduled[handle] = nil
      end

      local status = coroutine.status(co)
      if status == 'suspended' and wait_obj and wait_obj ~= self.KILL_THREAD then
         self._scheduled[handle] = nil
         self._waiting_until[handle] = wait_obj
      else
         table.insert(dead, handle)
      end
   end
  
   for _, handle in ipairs(dead) do
      radiant.events.trigger(handle, 'thread_exit')
   end
end

function ThreadsService:start_thread(fn)
   local handle = {
      thread_id = self._next_thread_id
   }
   self._next_thread_id = self._next_thread_id + 1

   local co = coroutine.create(fn)
   self._threads[handle] = co
   self._scheduled[handle] = co 
   return handle
end

function ThreadsService:suspend_thread(handle)
   assert(self._threads[handle] == self._running_thread)
   coroutine.yield(self.SUSPEND_THREAD)
end

function ThreadsService:resume_thread(handle)
   local wait_obj = self._waiting_until[handle]
   if wait_obj == self.SUSPEND_THREAD then
      self:schedule_thread(handle)
   end
end

-- removes the thread from the scheduler and the waiting thread
-- list.  If the thread is still running (terminate self?  is that
-- moral?), you still need to _complete_thread_termination later,
-- which will make sure the thread doesn't get rescheduled.
function ThreadsService:terminate_thread(handle)
   local co = self._threads[handle]
   if co then
      self._waiting_until[handle] = nil
      self._scheduled[handle] = nil
      self._threads[handle] = nil
      
      if self._running_thread ~= co then
         log:info('killing non running thread... nothing to do.')
      else 
         log:info('killing running thread... yielding KILL_THREAD.')
         coroutine.yield(self.KILL_THREAD)
      end
   end
end

function ThreadsService:wait_until(handle, cb)
   assert(self._threads[handle] == self._running_thread)
   self._waiting_until[handle] = cb
   self._scheduled[handle] = self._threads[handle]
end

function ThreadsService:schedule_thread(handle)
   assert(self._threads[handle] ~= nil)
   self._scheduled[handle] = self._threads[handle]
   self._waiting_until[handle] = nil
end

return ThreadsService()
