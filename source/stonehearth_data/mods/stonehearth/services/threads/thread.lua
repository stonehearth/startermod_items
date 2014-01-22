local log = radiant.log.create_logger('threads')
local next_thread_id = 1

local Thread = class()
Thread.SUSPEND = { name = "SUSPEND" }
Thread.KILL = { name = "KILL" }
Thread.DEAD = { name = "DEAD" }
Thread.WAIT = { name = "WAIT" }

function Thread:__init(parent)
   self._id = next_thread_id
   next_thread_id = next_thread_id + 1

   self._parent = parent
   self._scheduled = {}
   self._waiting_until = {}
   self._child_threads = {}
   self._msgs = {}
   self._log = radiant.log.create_logger('thread')
   self:set_debug_name('')
end

function Thread:get_id()
   return self._id
end

function Thread:set_debug_name(format, ...)
   local thread_route = ''
   local thread = self
   repeat
      thread_route = string.format('t:%d ', thread._id) .. thread_route
      thread = thread._parent
   until not thread or not thread._id

   self._debug_name = string.format(format, ...)
   self._log:set_prefix(thread_route .. self._debug_name)

   return self
end

function Thread:get_debug_route()
   local route = self._parent:get_debug_route()
   if route then
      return route .. ' ' .. self._debug_name
   end
   return self._debug_name
end

function Thread:set_thread_main(fn)
   self._thread_main = fn
   self._suspended = true
   return self
end

function Thread:set_msg_handler(handler)
   self._msg_handler = handler
   return self
end

function Thread:send_msg(...)
   local msg = {...}
   self._log:detail('adding msg "%s" to queue', tostring(msg[1]))
   table.insert(self._msgs, msg)
   self._parent:_schedule_thread(self)
end

function Thread:start(...)
   local args = {...}
   assert(self._thread_main)
   self._co = coroutine.create(function ()
         self:_process_queued_items()
         self._should_resume = true
         self._thread_main(unpack(args))
         self._should_resume = false
      end)
   if self._parent.is_running and self._parent:is_running() then
      self._log:detail('starting thread immedately')
      self._parent:_resume_child(self)
   else
      self._log:detail('starting thread')
      self:resume()
   end
   
   return self
end

function Thread:is_running()
   return Thread.running_thread == self
end

function Thread:create_thread()
   local thread = Thread(self)
   self._child_threads[thread:get_id()] = thread
   return thread
end

function Thread:suspend(reason)
   reason = tostring(reason) or 'no reason given'
   self._log:detail('suspending thread (%s)', reason)
   
   -- try to go to sleep.  we might not make it, though.  first give all our
   -- children a chance to run and pump all our pending messages.  anything in
   -- there could have called :resume().  if so, just return
   self._should_resume = false
   self:_process_queued_items()
   if self._should_resume then
      self._log:detail('early resumed from suspend (suspend reason: %s)', reason)
      return
   end
   self:_yield_thread(Thread.SUSPEND)
  
   -- and we're back!
   while true do 
      self:_process_queued_items()
      if self._should_resume then
         break
      else
         self._log:detail('going back to sleep...')
         self:_yield_thread(Thread.SUSPEND)
      end
   end

   -- finally, return.  this will resume execution of the client thread
   -- on the statement after they called suspend
   self._log:detail('resumed thread (suspend reason: %s)', reason)
end

function Thread:resume(reason)
   if self:is_running() then
      if not self._should_resume then
         self._log:detail('resume called when trying to suspend.  skipping the suspend.')
         self._should_resume = true
      end         
   else
      assert(not self._should_resume)
      self._should_resume = true
      self._parent:_schedule_thread(self)
   end
end

-- removes the thread from the scheduler and the waiting thread
-- list.  If the thread is still running (terminate self?  is that
-- moral?), you still need to _complete_thread_termination later,
-- which will make sure the thread doesn't get rescheduled.
function Thread:terminate(reason)
   if self:is_running() then
      self:_yield_thread(Thread.DEAD)
   else
      self._parent:_terminate(self)
   end
end

--[[
function Thread:wait_until(handle, cb)
   assert(self._threads[handle] == self._running_thread)
   self._waiting_until[handle] = cb
   self._scheduled[handle] = self._threads[handle]
end
]]

function Thread:_schedule_thread(thread)  
   local id = thread:get_id()
   assert(self._child_threads[id] == thread)

   if not self._scheduled[id] then
      self._log:detail('scheduling child thread %d', id)
      self._parent:_schedule_thread(self)
      self._scheduled[id] = thread
   end
   self._waiting_until[id] = nil
end

function Thread:_resume_thread()
   assert(self._co)
   assert(not self:is_running())

   self._last_running_thread = Thread.running_thread
   Thread.running_thread = self

   return coroutine.resume(self._co)
end

function Thread:_yield_thread(...)
   assert(self._co)
   assert(self:is_running())

   Thread.running_thread = self._last_running_thread
   coroutine.yield(...)

   assert(self:is_running())
end

function Thread:_resume(now)
   assert(self._co)
   assert(self._suspended)
   self._suspended = false

   local success, thread_status, wait_obj = self:_resume_thread()
   if not success then
      radiant.check.report_thread_error(self._co, 'thread error: ' .. tostring(wait_obj))
      self._co = nil
      return Thread.DEAD
   end

   local status = coroutine.status(self._co)
   if status == 'dead' then
      return Thread.DEAD
   elseif status == 'suspended' then
      self._suspended = true
      return thread_status, wait_obj
   else
      error(string.format('unknown status "%s" returned from coroutine.status', status))
   end
end

function Thread:_resume_child(thread)
   self._log:detail('resuming child thread %d', thread._id)
   assert(self._child_threads[thread:get_id()] == thread)
   
   local status, wait_obj = thread:_resume()
   
   if status == Thread.DEAD then
      self._scheduled[id] = nil
      self._child_threads[id] = nil
   elseif status == Thread.SUSPEND then
      -- nothing to do...
   elseif status == Thread.WAIT then
      error('wait not implemented')
      self._waiting_until[id] = wait_obj         
   elseif status == Thread.KILL then
      error('kill not implemented')
   else
      error('unknown status returned from thread')
   end
end

function Thread:_resume_children(now)
   --[[
   for id, thread in pairs(self._waiting_until) do
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
   ]]

   self._log:spam('starting resume children loop.')
   repeat
      local scheduled = self._scheduled
      self._scheduled = {}
      for id, thread in pairs(scheduled) do
         self:_resume_child(thread)
      end
      if next(self._scheduled) then
         self._log:detail('repeating resume children loop.')
      end      
   until next(self._scheduled) == nil
   self._log:spam('finished resume children loop')
end

function Thread:_dispatch_messages()
   -- keep draining the queue till it's empty
   self._log:spam('starting dispatch message loop.')
   while #self._msgs > 0 do  
      local msgs = self._msgs
      self._msgs = {}
      for _, msg in ipairs(msgs) do
         self._log:detail('dispatching msg "%s"', tostring(msg[1]))
         self._msg_handler(unpack(msg))
      end
   end
   self._log:spam('finished dispatch message loop.')
end

function Thread:_process_queued_items()
   repeat
      -- first, give all our children a chance to run
      self:_resume_children()
      assert(next(self._scheduled) == nil)
      
      -- now process all our messages...
      self:_dispatch_messages()
      assert(next(self._msgs) == nil)
   until next(self._scheduled) == nil and next(self._msgs) == nil
end

return Thread
