local TraceCategories = _radiant.dm.TraceCategories

local ObjectMonitor = class()

function ObjectMonitor:__init(log)
   self._log = log
   self._traces = {}
end

function ObjectMonitor:destroy()
   for _, trace in pairs(self._traces) do
      trace:destroy()
   end
   self._traces = {}
end

function ObjectMonitor:resume()
   self._paused = false
   if self._triggered then
      self:_on_destroyed()
   end
end

function ObjectMonitor:pause()
   self._paused = true
end

function ObjectMonitor:is_running()
   return not self._paused
end

function ObjectMonitor:protect_object(obj)
   assert(obj and obj:is_valid())
   local id = obj:get_id()

   local name = tostring(obj)   
   local trace = obj:trace('object monitor', TraceCategories.SYNC_TRACE)
                        :on_destroyed(function()
                              self:_on_destroyed(name)
                           end)
   self._traces[id] = trace
end

function ObjectMonitor:unprotect_object(obj)
   assert(obj and obj:is_valid())
   local id = obj:get_id()
   local trace = self._traces[id]

   if trace then
      trace:destroy()
      self._traces[id] = nil
   end
end

function ObjectMonitor:set_destroyed_cb(cb)
   self._destroyed_cb = cb
end

function ObjectMonitor:_on_destroyed(name)
   self:destroy()
   self._triggered = true

   if not self._paused then
      if self._destroyed_cb then
         self._log:detail('object monitor calling destroyed cb (%s is no more!)',  name)
         self._destroyed_cb(name)
      end
   end
end

return ObjectMonitor
