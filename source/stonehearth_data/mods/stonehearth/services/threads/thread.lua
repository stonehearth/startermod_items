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
   self:set_debug_name('t')
end

function Thread:get_id()
   return self._id
end

function Thread:set_debug_name(format, ...)
   self._debug_name = string.format(format, ...)
   self._log:set_prefix(string.format('%s thread', self:get_debug_route())
   
   return self
end

function Thread:get_debug_route()
   return self._parent:get_debug_route() .. ' ' .. self._debug_name
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
   self._parent:_schedule_thread(self)
   local msg = {...}
   self._log:detail('adding msg "%s" to queue', tostring(msg[1]))
   table.insert(self._msgs, msg)
end

function Thread:start(...)
   local args = {...}
   assert(self._thread_main)
   self._co = coroutine.create(function ()
         self._parent:_schedule_thread(self)
         self._should_resume = true
         self._thread_main(unpack(args))
         self._should_resume = false
      end)
   self._log:detail('starting thread')
   self:resume()
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
   self._log:detail('suspending thread (%s)', tostring(reason or 'no reason given'))
   
   self._should_resume = false
   self:_yield_thread(Thread.SUSPEND)

   self._log:detail('resumed thread (suspend reason: %s)', tostring(reason or 'no reason given'))
   
   -- and we're back!
   while true do 
      -- first, give all our children a chance to run
      self:_resume_children()
      assert(next(self._scheduled) == nil)
      
      -- now process all our messages...
      self:_dispatch_messages()

      -- sometimes we wake up just to pump messages or give our children a
      -- go.  if that's the case, go back to sleep.  If someone actually called
      -- :resume(), exit the loop so we can continue
      if self._should_resume then
         break
      else
         -- before we yield, make sure all our children have yielded and
         -- we have completely drained the msg queue
         assert(next(self._msgs) == nil)
         assert(next(self._scheduled) == nil)        
         self._log:detail('going back to sleep...')
         self:_yield_thread(Thread.SUSPEND)
      end
   end

   -- finally, return.  this will resume execution of the client thread
   -- on the statement after they called suspend
end

function Thread:resume(reason)
   assert(not self:is_running())
   assert(not self._should_resume)
   self._should_resume = true
   self._parent:_schedule_thread(self)
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
      self._parent:_schedule_thread(self)
   end
   self._scheduled[id] = thread
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

   repeat
      local scheduled = self._scheduled
      self._scheduled = {}
      for id, thread in pairs(scheduled) do
      
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
   until next(self._scheduled) == nil
end

function Thread:_dispatch_messages()
   -- keep draining the queue till it's empty
   while #self._msgs > 0 do  
      local msgs = self._msgs
      self._msgs = {}
      for _, msg in ipairs(self._msgs) do
         self._msg_handler(unpack(msg))
      end
   end
end


return Thread
