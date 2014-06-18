local log = radiant.log.create_logger('thread')
local next_thread_id = 1

local Thread = class()
Thread.SUSPEND = { name = "SUSPEND" }
Thread.KILL = { name = "KILL" }

Thread.all_threads = {}
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

function Thread.get_current_thread()
   local co = coroutine.running()
   if co then
      return Thread.all_threads[co]
   end
   return nil
end

function Thread.schedule_thread(thread)
   assert(not thread._finished)
   if not Thread.is_scheduled[thread] then
      thread._log:detail('adding thread to scheduled list')
      Thread.is_scheduled[thread] = true
      table.insert(Thread.scheduled, thread)
   end
end

function Thread.resume_thread(thread)
   local success, thread_status, result1 = thread:_do_resume()
   assert(success, 'thread coroutine failed to resume')

   local status = coroutine.status(thread._co)
   if status == 'dead' then
      Thread.terminate_thread(thread)
      return
   elseif status == 'suspended' then
      if thread_status == Thread.SUSPEND then
         -- nothing to do...
      elseif thread_status == Thread.KILL then
         Thread.terminate_thread(thread, result1)
      else
         error(string.format('unknown thread_status "%s" returned from resume', tostring(thread_status)))
      end
   else
      error(string.format('unknown thread state "%s" returned from resume', tostring(status)))
   end
end

function Thread.wait_thread(thread)
   local current_thread = Thread.get_current_thread()
   assert(current_thread)
   assert(not thread:is_running())

   local log = current_thread._log
   thread:wait_for_children()
   if not thread._finished then
      log:detail('thread %d still not finished.  entering spin loop', thread._id)
   end
   while not thread._finished do
      local waiters = Thread.waiting_for[thread._id]
      if not waiters then
         waiters = {}
         Thread.waiting_for[thread._id] = waiters
      end
      waiters[current_thread._id] = current_thread
      current_thread:suspend(string.format('%d waiting for thread %d to finish', current_thread._id, thread._id))
   end
end

function Thread.terminate_thread(thread, err)
   if not thread._co then
      -- never started, so there's nothing to do.
      return
   end
   
   Thread.all_threads[thread._co] = nil
   Thread.scheduled[thread._id] = nil
   thread:_on_thread_exit(err)

   local waiters = Thread.waiting_for[thread._id] or {}
   Thread.waiting_for[thread._id] = nil

   for _, waiter in pairs(waiters) do
      waiter:resume('resuming thread %d (waiting for %d)', waiter._id, thread._id)
   end
end

function Thread:__init(parent)
   self._id = next_thread_id
   next_thread_id = next_thread_id + 1

   self._parent = parent
   self._child_threads = {}
   self._msgs = {}
   self._thread_data = {}
   self._exit_handlers = {}
   self._log = radiant.log.create_logger('thread')
   self:set_debug_name('')
end


function Thread:get_id()
   return self._id
end

function Thread:set_thread_data(key, data)
   self._thread_data[key] = data
   return self
end

function Thread:get_thread_data(key, data, default)
   return self._thread_data[key] or default
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

function Thread:add_exit_handler(handler)
   table.insert(self._exit_handlers, handler)
   return self
end

function Thread:create_thread()
   local thread = Thread(self)
   self._child_threads[thread:get_id()] = thread
   return thread
end

function Thread:send_msg(...)
   local msg = {...}

   if self._finished then
      self._log:debug('ignoring msg "%s" sent to dead thread', msg[1])
      return
   end
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
         -- make a pcall.. this only works because we've installed coco
         local error_handler = function(err)
            local traceback = debug.traceback()
            _host:report_error(err, traceback)
            Thread.terminate_thread(self)
            return err
         end
         xpcall(function()
               self:_dispatch_messages()
               self._should_resume = true
               self._thread_main(unpack(args))
               self._should_resume = false
            end, error_handler)
      end)
   Thread.all_threads[self._co] = self

   self._log:detail('starting thread')   
   Thread.resume_thread(self)

   return self
end

function Thread:report_thread_error(err)
   radiant.check.report_thread_error(self._co, 'thread error: ' .. err)   
end

function Thread:interrupt(fn)
   if not self._finished then
      local current = Thread.get_current_thread()
      self._log:detail('thread %s is currently running in :interrupt', current and tostring(current:get_id()) or 'nil')
      if self:is_running() then
         -- if we're actually running now, ai:interrupt() will call fn() immediately.
         fn()
      else
         self:send_msg('thread:call_interrupt', fn)
         if coroutine.status(self._co) == 'suspended' then
            -- if we're suspended (i.e. not actually running at all!), ai:interrupt() will
            -- schedule and run fn() immediately.
            self._log:detail('switching to thread immediately to deliver interrupt.')
            Thread.resume_thread(self)
            self._log:detail('finished delivering interrupt.')
         else
            -- if we're in a running state but NOT the current thread (e.g. the
            -- ai thread does something which triggers a cpp data trace (e.g. modifying a
            -- region).  cpp data traces are deliever in the special callback thread so they
            -- don't screw up the current interperter state in the event of an error).  in
            -- cases like these, it's IMPOSSIBLE to get back to the thread immediately.
            --
            -- if we can't get back immediately, *anything* could happen before the
            -- thread suspends and processes the interrupt message.  it's completely unsafe
            -- to call fn there!  the only solution is to modify the rest of the entire codebase to
            -- guarantee this NEVER happens.  for example, all the bits which uninject
            -- actions (e.g. tasks, equipment pieces, buffs, etc.) need to do so from the
            -- main lua thread and not from cpp callbacks (ideally).
            assert(false, 'invalid thread state in Thread:interrupt (see comment).')
         end
      end
   end
end

function Thread:_call_interrupt(fn)
   assert(self:is_running())
   fn()
end

function Thread:is_finished()
   return self._finished
end

function Thread:wait()
   local log = Thread.get_current_thread()._log
   log:detail('thread %d waiting for thread %d to finish', Thread.get_current_thread()._id, self._id)
   Thread.wait_thread(self)
   log:detail('returning from wait for %d', self._id)
end

function Thread:wait_for_children()
   local log = Thread.get_current_thread()._log
   
   log:detail('waiting for children to finish...')
   for id, _ in pairs(self._child_threads) do
      log:spam('   %d still not finished...', id)
   end
   local id, first_child = next(self._child_threads)
   while first_child do
      first_child:wait()
      id, first_child = next(self._child_threads)
   end
   log:detail('all children have finished.')
   assert(not next(self._child_threads) )
end

function Thread:get_thread_status()
   return self._co and coroutine.status(self._co) or 'dead'
end

function Thread:is_running()
   return Thread.get_current_thread() == self
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
   elseif not self._should_resume then
      self._should_resume = true
      Thread.schedule_thread(self)
   end
end

function Thread:sleep_realtime(ms)
   local wakeup = radiant.get_realtime() + (ms / 1000.0)
   local start_ms = radiant.get_realtime() * 1000
   local timer
   repeat
      if timer then
         timer:destroy()
      end
      timer = radiant.set_realtime_timer(ms, function()
            if not self:is_finished() then
               self:resume()
            end
         end)
      self:suspend()
      ms = ms - ((radiant.get_realtime() * 1000) - start_ms)
   until self:is_finished() or radiant.get_realtime() >= wakeup
   timer:destroy()
end


-- removes the thread from the scheduler and the waiting thread
-- list.  If the thread is still running (terminate self?  is that
-- moral?), you still need to _complete_thread_termination later,
-- which will make sure the thread doesn't get rescheduled.
function Thread:terminate(err)
   assert(not self._finished)
   self._log:detail('terminating thread: %s', err or '(no error)')
   if self:is_running() then
      self:_do_yield(Thread.KILL, err)
      radiant.not_reached('no return after yielding Thread.KILL')
   else
      Thread.terminate_thread(self, err)
   end
end

function Thread:_do_resume()
   assert(self._co, 'thread has no coroutine in resume')
   assert(not self:is_running(), 'thread is not running in resume')
   if coroutine.status(self._co) ~= 'suspended' then
      local co = coroutine.running()
      local thread = Thread.all_threads[co]
   end
   assert(coroutine.status(self._co) == 'suspended', 'thread coroutine not suspened in _do_resume()')

   self._log:spam('coroutine.resume (status:%s).', coroutine.status(self._co))
   return coroutine.resume(self._co)
end

function Thread:_do_yield(...)
   assert(self._co)
   assert(self:is_running())

   coroutine.yield(...)

   -- if we got terminated while suspended, just yield indefinitely.
   -- we'll be destroyed when the last reference goes away
   while self._finished do
      coroutine.yield(Thread.SUSPEND)
      self._log:spam('coroutine resumed after thread died (weird!).  yielding.')
   end
   self._log:detail('coroutine returned from yield.')

   assert(self:is_running())
end

function Thread:_child_finished(thread)
   local id = thread:get_id()
   assert(self._child_threads[id] == thread)
   self._child_threads[id] = nil
end

function Thread:_on_thread_exit(err)
   self._log:detail('thread %d has finished', self._id)
   self._finished = true
   self._msgs = {}
   for _, fn in ipairs(self._exit_handlers) do
      fn(self, err)
   end
   
   if self._parent then
      self._parent:_child_finished(self)
      self._parent:send_msg('child_thread_exit', self, err)
   end
end

function Thread:_dispatch_messages()
   -- keep draining the queue till it's empty
   self._log:spam('starting dispatch message loop.')
   while #self._msgs > 0 do  
      local msgs = self._msgs
      self._msgs = {}
      for _, msg in ipairs(msgs) do
         local msg_name = msg[1]
         self._log:detail('dispatching msg "%s"', tostring(msg_name))
         local private_msg = Thread.private_messages[msg_name]
         if private_msg then
            private_msg(self, select(2, unpack(msg)))
         elseif self._msg_handler then
            self._msg_handler(unpack(msg))
         end
         
         if self._finished then
            break
         end
      end
   end
   self._log:spam('finished dispatch message loop.')
end

Thread.private_messages = {
   ['thread:call_interrupt'] = Thread._call_interrupt,
}

return Thread

