local RequestDispatcher = class()

function RequestDispatcher:__init(promise)
   self.on = {} -- filled in by request creator
   self._recv_queue = {}
   self._suspend_until_realtime = 0
   self:set_promise(promise)
end

function RequestDispatcher:set_promise(promise)
   if promise then
      self._promise = promise
      self._promise:progress(function(...)
            local msg = ...
            table.insert(self._recv_queue, msg)
         end)
      self._queue = {}

      radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._process_queue)
   end
end

function RequestDispatcher:pause_for_realtime(ms)
   self._suspend_until_realtime = _host:get_realtime() + ms
end

function RequestDispatcher:_process_queue()
   local now = _host:get_realtime()
   while now > self._suspend_until_realtime and #self._recv_queue > 0 do
      local msg = table.remove(self._recv_queue, 1)
      local cmd = table.remove(msg, 1)
      if not self.on[cmd] then
         autotest.fail('cannot dispatch command: %s', tostring(cmd))
         return
      end
      self.on[cmd](unpack(msg))
   end
end

return RequestDispatcher
