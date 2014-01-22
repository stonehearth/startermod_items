local log = radiant.log.create_logger('threads')
local next_thread_id = 1

local Thread = class()
Thread.SUSPEND = { name = "SUSPEND" }
Thread.KILL = { name = "KILL" }
Thread.DEAD = { name = "DEAD" }

Thread.scheduled = {}
Thread.is_scheduled = {}
Thread.waiting_for = {}

function Thread.new()
   return Thread()
end

function Thread.loop()
   while #Thread.scheduled > 0 do
      log:spam('starting thread loop')
      local scheduled = Thread.scheduled
      Thread.scheduled, Thread.is_scheduled = {}, {}
      for _, thread in ipairs(scheduled) do
         Thread.resume_thread(thread)
      end
      log:spam('finished thread loop')
   end
end

function Thread.schedule_thread(thread)
   assert(not thread._finished)
   if not Thread.is_scheduled[thread] then
      thread._log:detail('scheduling thread')
      Thread.is_scheduled[thread] = true
      table.insert(thread.scheduled, thread)
   end
end

function Thread.resume_thread(thread)
   local status, err = thread:_resume()
   
   if status == Thread.DEAD then
      Thread.terminate_thread(thread)
   elseif status == Thread.SUSPEND then
      -- nothing to do...
   elseif status == Thread.KILL then
      Thread.terminate_thread(thread, err)
   else
      error('unknown status returned from thread')
   end
end

function Thread.wait_thread(thread)
   local current_thread = Thread.current_thread
   assert(current_thread)
   assert(not thread:is_running())

   thread:wait_for_children()
   while not thread._finished do
      local waiters = Thread.waiting_for[thread._id]
      if not waiters then
         waiters = {}
         Thread.waiting_for[thread._id] = waiters
      end
      waiters[current_thread._id] = current_thread
      current_thread:suspend(string.format('waiting for thread %d to finish', thread._id))
   end
end

function Thread.terminate_thread(thread, err)
   Thread.scheduled[thread._id] = nil
   thread:_on_thread_exit(err)

   local waiters = Thread.waiting_for[thread._id] or {}
   Thread.waiting_for[thread._id] = nil

   for _, waiter in pairs(waiters) do
      waiter:resume('wait for %d likely to succeed', thread._id)
   end
end

function Thread:__init(parent)
   self._id = next_thread_id
   next_thread_id = next_thread_id + 1

   self._parent = parent
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
   if self._parent then
      return self._parent:get_debug_route() .. ' ' .. self._debug_name
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

function Thread:set_exit_handler(handler)
   assert(not self._finished)
   self._exit_handler = handler
   return self
end

function Thread:create_thread()
   local thread = Thread(self)
   self._child_threads[thread:get_id()] = thread
   return thread
end

function Thread:send_msg(...)
   assert(not self._finished)

   local msg = {...}
   self._log:detail('adding msg "%s" to queue', tostring(msg[1]))
   table.insert(self._msgs, msg)

   -- if the thread is currently running we'll pull the message out of the queue
   -- and dispatch it before it has a chance to suspend.  if it's not running, ask
   -- the Thread singleton to schedule us and we'll dispatch it then.
   if not self:is_running() then
      Thread.schedule_thread(self)
   end
end

function Thread:start(...)
   local args = {...}
   assert(self._thread_main)
   assert(not self._finished)

   self._co = coroutine.create(function ()
         self:_dispatch_messages()
         self._should_resume = true
         self._thread_main(unpack(args))
         self._should_resume = false
      end)
   self._log:detail('starting thread')   
   Thread.resume_thread(self)

   return self
end

function Thread:wait()
   self._log:detail('waiting for thread %d to finish', self._id)
   Thread.wait_thread(self)
   self._log:detail('returning from wait for %d', self._id)
end

function Thread:wait_for_children()
   self._log:detail('waiting for children to finish...')
   local id, first_child = next(self._child_threads)
   while first_child do
      first_child:wait()
      first_child = next(self._child_threads)
   end
   self._log:detail('all children have finished.')
   assert(not next(self._child_threads) )
end

function Thread:is_running()
   return Thread.current_thread == self
end

function Thread:suspend(reason)
   assert(not self._finished)

   reason = tostring(reason) or 'no reason given'
   self._log:detail('suspending thread (%s)', reason)
   
   -- try to go to sleep.  we might not make it, though.  first give all our
   -- children a chance to run and pump all our pending messages.  anything in
   -- there could have called :resume().  if so, just return
   self._should_resume = false
   self:_dispatch_messages()
   if self._should_resume then
      self._log:detail('early resumed from suspend (suspend reason: %s)', reason)
      return
   end
   self:_do_yield(Thread.SUSPEND)
  
   -- and we're back!
   while true do 
      self:_dispatch_messages()
      if self._should_resume then
         break
      else
         self._log:spam('going back to sleep...')
         self:_do_yield(Thread.SUSPEND)
      end
   end

   -- finally, return.  this will resume execution of the client thread
   -- on the statement after they called suspend
   self._log:detail('resumed thread (suspend reason: %s)', reason)
end

function Thread:resume(reason)
   assert(not self._finished)

   if self:is_running() then
      if not self._should_resume then
         self._log:detail('resume called when trying to suspend.  skipping the suspend.')
         self._should_resume = true
      end         
   else
      assert(not self._should_resume)
      self._should_resume = true
      Thread.schedule_thread(self)
   end
end

-- removes the thread from the scheduler and the waiting thread
-- list.  If the thread is still running (terminate self?  is that
-- moral?), you still need to _complete_thread_termination later,
-- which will make sure the thread doesn't get rescheduled.
function Thread:terminate(err)
   assert(err)
   assert(not self._finished)
   self._log:detail('terminating thread: %s', err)
   if self:is_running() then
      self:_do_yield(Thread.KILL, err)
      radiant.not_reached('no return after yielding Thread.DEAD')
   else
      Thread.terminate_thread(self)
   end
end

function Thread:_do_resume()
   assert(self._co)
   assert(not self:is_running())

   self._last_current_thread = Thread.current_thread
   Thread.current_thread = self

   self._log:spam('coroutine.resume...')
   return coroutine.resume(self._co)
end

function Thread:_do_yield(...)
   assert(self._co)
   assert(self:is_running())

   Thread.current_thread = self._last_current_thread
   coroutine.yield(...)

   assert(self:is_running())
end

function Thread:_child_finished(thread)
   local id = thread:get_id()
   assert(self._child_threads[id] == thread)
   self._child_threads[id] = nil
end

function Thread:_on_thread_exit(err)
   if self._parent then
      self._parent:_child_finished(self)
   end
   self._finished = true
   if self._exit_handler then
      self._exit_handler(self, err)
   end
end

function Thread:_resume()
   assert(self._co)
   assert(self._suspended)
   assert(not self._finished)

   self._suspended = false

   local success, thread_status, wait_obj = self:_do_resume()
   if not success then
      radiant.check.report_thread_error(self._co, 'thread error: ' .. tostring(thread_status))
      self._co = nil
      return Thread.KILL, thread_status
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

return Thread

