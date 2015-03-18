local TraceCategories = _radiant.dm.TraceCategories

local ObjectMonitor = class()

function ObjectMonitor:__init(log)
   self._log = log
   self._log:spam('constructing object monitor')
   self._traces = {}
end

function ObjectMonitor:destroy()
   self._log:spam('destroying object monitor')
   for _, trace in pairs(self._traces) do
      trace:destroy()
   end
   self._traces = {}
end

function ObjectMonitor:resume()
   self._log:spam('resuming object monitor')
   self._paused = false
   if self._triggered then
      self:_on_destroyed()
   end
end

function ObjectMonitor:pause()
   self._log:spam('pausing object monitor')
   self._paused = true
end

function ObjectMonitor:is_running()
   return not self._paused
end

function ObjectMonitor:protect_object(obj)
   assert(type(obj) == 'userdata')
   assert(obj and obj:is_valid())
   local id = obj:get_id()
   local name = tostring(obj)

   self._log:detail('object monitor monitoring %s', name)
   local trace = obj:trace('object monitor', TraceCategories.SYNC_TRACE)
                        :on_destroyed(function()
                              self:_on_destroyed(name)
                           end)
   self._traces[id] = trace
end

function ObjectMonitor:unprotect_object(obj)
   assert(type(obj) == 'userdata')
   assert(obj and obj:is_valid())
   local id = obj:get_id()
   local trace = self._traces[id]

   self._log:detail('object monitor monitoring %s', obj)

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

   if self._paused then
      self._log:debug('ignoring destroy of %s (paused!)', name)
      return
   end

   if not self._destroyed_cb then
      self._log:debug('ignoring detroy of %s (no destroy cb!)', name)
   end

   self._log:debug('object monitor calling destroyed cb (%s is no more!)',  name)
   self._destroyed_cb(name)
end

return ObjectMonitor
