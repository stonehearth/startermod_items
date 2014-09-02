local FilteredTrace = class()

-- This class wraps a generic C++ trace and filters the on_changed event with the supplied filter_fn.
function FilteredTrace:__init(trace_impl, filter_fn)
   self._trace_impl = trace_impl
   self._filter_fn = filter_fn
end

function FilteredTrace:on_changed(changed_cb)
   local filter_cb = nil

   self._changed_cb = changed_cb

   if changed_cb then
      filter_cb = function()
            if self._filter_fn() then
               changed_cb()
            end
         end
   end

   self._trace_impl:on_changed(filter_cb)
   return self
end

function FilteredTrace:on_destroyed(destroyed_cb)
   self._trace_impl:on_destroyed(destroyed_cb)
   return self
end

function FilteredTrace:push_object_state()
   if self._changed_cb then
      self._changed_cb()
   end
   return self
end

function FilteredTrace:destroy()
   if self._trace_impl then
      self._trace_impl:destroy()
      self._trace_impl = nil
   end

   self._filter_fn = nil
   self._changed_cb = nil
end

return FilteredTrace
