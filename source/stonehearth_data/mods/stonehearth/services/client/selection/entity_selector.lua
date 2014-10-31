local EntitySelector = class()
local Point3 = _radiant.csg.Point3


local OFFSCREEN = Point3(0, -100000, 0)

function EntitySelector:done(cb)
   self._done_cb = cb
   return self
end

function EntitySelector:progress(cb)
   self._progress_cb = cb
   return self
end

function EntitySelector:fail(cb)
   self._fail_cb = cb
   return self
end

function EntitySelector:always(cb)
   self._always_cb = cb
   return self
end

function EntitySelector:_call_once(name, ...)
   local method_name = '_' .. name .. '_cb'
   if self[method_name] then
      local method = self[method_name]
      self[method_name] = nil
      method(self, ...)
   end
end

function EntitySelector:resolve(...)
   self:_call_once('done', ...)
   self:_call_once('always')
   self:_cleanup()
   return self
end

function EntitySelector:reject(...)
   self:_call_once('fail', ...)
   self:_call_once('always')
   self:_cleanup()
   return self
end

function EntitySelector:notify(...)
   if self._progress_cb then
      self._progress_cb(self, ...)
   end
   return self
end

function EntitySelector:_cleanup()
   stonehearth.selection:register_tool(self, true)

   self._fail_cb = nil
   self._progress_cb = nil
   self._done_cb = nil
   self._always_cb = nil

   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end

   if self._cursor_obj then
      self._cursor_obj:destroy()
      self._cursor_obj = nil
   end
end

function EntitySelector:set_filter_fn(fn)
   self._filter_fn = fn
   return self
end

function EntitySelector:set_cursor(uri)
   self._cursor_uri = uri
   return self
end

-- given the x, y coordinate on screen, return the brick that is a
-- candidate for selection.  trace a ray through that point in the screen,
-- ignoring nodes between ourselves and the desired brick (e.g. the cursor
-- is almost certainly going to get in the way!)
--
function EntitySelector:_get_selected_entity(x, y)
   -- query the scene to figure out what's under the mouse cursor
   local s = _radiant.client.query_scene(x, y)

   for result in s:each_result() do
      -- skip the cursor...
      local entity = result.entity
      if entity and (not self._filter_fn or self._filter_fn(entity)) then
         return entity
      end
   end
end

-- handle mouse events from the input service.  basically moves the cursor
-- around and potentially calls the promise handlers to notify the client
-- of interesting events
--
function EntitySelector:_on_mouse_event(mouse_pos, event)
   assert(self._input_capture, "got mouse event after releasing capture")

   if event and event:up(2) and not event.dragging then
   -- if the user right clicks, cancel the selection
      self:reject({ error = 'cancelled via right click'})
      return
   end

   local entity = self:_get_selected_entity(mouse_pos.x, mouse_pos.y)

   -- if the user installed a progress handler, go ahead and call it now
   self:notify(entity)

   local cursor_uri = entity and self._cursor_uri or 'stonehearth:cursors:invalid_hover'
   if self._last_cursor_uri ~= cursor_uri then
      if self._cursor_obj then
         self._cursor_obj:destroy()
      end
      self._last_cursor_uri = cursor_uri
      self._cursor_obj = _radiant.client.set_cursor(cursor_uri)
   end

   if entity and event and event:up(1) then
      self:resolve(entity)
   end
end

-- run the location selector.   should be called after the selector
-- has been configured (cursor entity, etc.).  be sure to call
-- :destroy() when you no longer need the selection active.
--
function EntitySelector:go()
   self._rotation = 0
   stonehearth.selection:register_tool(self, true)

   -- capture the mouse.  Call our _on_mouse_event each time, passing in
   -- the entity that we're supposed to create whenever the user clicks.
   self._input_capture = stonehearth.input:capture_input()
                           :on_mouse_event(function(e)
                                 self:_on_mouse_event(e, e)
                                 return true
                              end)

   self._last_cursor_uri = self._cursor_uri
   self._cursor_obj = _radiant.client.set_cursor(self._cursor_uri)
   -- fake an initial event to move the cursor under the mouse
   if self._progress_cb then
      self:_on_mouse_event(_radiant.client.get_mouse_position())
   end
   return self
end

return EntitySelector
