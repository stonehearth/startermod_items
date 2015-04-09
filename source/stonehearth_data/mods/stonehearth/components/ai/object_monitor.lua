local TraceCategories = _radiant.dm.TraceCategories

local function _destroy(self)
   self._log:spam('destroying object monitor')
   for _, trace in pairs(self._traces) do
      trace:destroy()
   end
   self._traces = {}
end

local function _resume(self)
   self._log:spam('resuming object monitor')
   self._paused = false
   if self._triggered then
      self:_on_destroyed()
   end
end

local function _pause(self)
   self._log:spam('pausing object monitor')
   self._paused = true
end

local function _is_running(self)
   return not self._paused
end

local function _protect_object(self, obj)
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

local function _unprotect_object(self, obj)
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

local function _set_destroyed_cb(self, cb)
   self._destroyed_cb = cb
end

local function _on_destroyed(self, name)
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

local function _new(log)
   log:spam('constructing object monitor')
   
   local om = {
      _log = log,
      _traces = {},

      destroy = _destroy,
      resume = _resume,
      pause = _pause,
      is_running = _is_running,
      protect_object = _protect_object,
      unprotect_object = _unprotect_object,
      set_destroyed_cb = _set_destroyed_cb,
      _on_destroyed = _on_destroyed,
   }

   return om
end

local ObjectMonitor = {
   new = _new,
}

return ObjectMonitor
