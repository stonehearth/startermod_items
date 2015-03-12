local FilteredTrace = class()

-- This class wraps a generic C++ trace and filters the on_changed event with the supplied filter_fn.
function FilteredTrace:__init(trace_impl, filter_fn)
   self._trace_impl = trace_impl

   self._trace_impl:on_changed(function(...)
         if self._changed_cb then
            local results = { filter_fn(...) }
            local first_result = results[1]
            if first_result ~= nil and first_result ~= false then
               self._changed_cb(unpack(results))
            end
         end
      end)   
end

function FilteredTrace:on_changed(changed_cb)
   self._changed_cb = changed_cb
   return self
end

function FilteredTrace:on_destroyed(destroyed_cb)
   self._trace_impl:on_destroyed(destroyed_cb)
   return self
end

function FilteredTrace:push_object_state()
   self._trace_impl:push_object_state()
   return self
end

function FilteredTrace:destroy()
   if self._trace_impl then
      self._trace_impl:destroy()
      self._trace_impl = nil
   end
end

return FilteredTrace
