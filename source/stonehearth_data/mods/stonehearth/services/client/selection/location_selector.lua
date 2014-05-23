local LocationSelector = class()
local Point3 = _radiant.csg.Point3


local OFFSCREEN = Point3(0, -100000, 0)

function LocationSelector:done(cb)
   self._done_cb = cb
   return self
end

function LocationSelector:progress(cb)
   self._progress_cb = cb
   return self
end

function LocationSelector:fail(cb)
   self._fail_cb = cb
   return self
end

function LocationSelector:always(cb)
   self._always_cb = cb
   return self
end

function LocationSelector:allow_shift_queuing(enabled)
   self._allow_shift_queuing = enabled
   return self
end

function LocationSelector:set_filter_fn(fn)
   self._filter_fn = fn
   return self
end

function LocationSelector:set_min_locations_count(count)
   self._locations_remaining_count = count
   return self
end

-- sets the uri of the entity to use for the ghost cursor.  this entity's lifetime
-- will be controlled by the selection service.  it will also automatically be
-- rendered in ghostly form.  if you want more control over how the cursor entity
-- gets rendered, make it yourself and use set_cursor_entity()
--
--    @param uri - the uri to the type of entity to create as the cursor
--
function LocationSelector:use_ghost_entity_cursor(uri)   
   local cursor_entity = radiant.entities.create_entity(uri)
   cursor_entity:add_component('render_info')
                     :set_material('materials/ghost_item.xml')
   self:set_cursor_entity(cursor_entity)
   return self
end

-- sets the entity to be used as the cursor while choosing a location.  the
-- location selector takes ownership of this entity.  it will automatically be
-- destroyed when the selection finishes.  for this reason, the cursor entity
-- must have been created in the client temporary store.
--
--    @param cursor_entity - the entity to use for the cursor
--
function LocationSelector:set_cursor_entity(cursor_entity)   
   self._cursor_entity = cursor_entity
   self._cursor_render_entity = _radiant.client.create_render_entity(1, cursor_entity)
   return self
end

-- destroy the location selector.  needs to be called when the user is
-- "done" with the selection, usually in an :always() promise callback,
-- though the user may choose to keep it around till later if they'd like
-- the cursor entity to stick around (e.g. in a multi-step wizard)
--
function LocationSelector:destroy()
   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end
   if self._cursor_entity then
      radiant.entities.destroy_entity(self._cursor_entity)
      self._cursor_entity = nil
   end 
end

-- given the x, y coordinate on screen, return the brick that is a
-- candidate for selection.  trace a ray through that point in the screen,
-- ignoring nodes between ourselves and the desired brick (e.g. the cursor
-- is almost certainly going to get in the way!)
--
function LocationSelector:_get_selected_brick(x, y)
   -- query the scene to figure out what's under the mouse cursor
   local s = _radiant.client.query_scene(x, y)

   for result in s:each_result() do
      -- skip the cursor...
      if result.entity ~= self._cursor_entity then
         if not self._filter_fn or self._filter_fn(result) then
            local brick, normal = result.brick, result.normal
            if normal.x > 0 or normal.z > 0 or normal.y > 0 then
               return brick + normal:to_int()
            end
            return brick
         end
      end
   end
end

function LocationSelector:_shift_down()
  return _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_LEFT_SHIFT) or
         _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_RIGHT_SHIFT)
end

-- handle mouse events from the input service.  basically moves the cursor
-- around and potentially calls the promise handlers to notify the client
-- of interesting events
--
function LocationSelector:_on_mouse_event(mouse_pos, event)
   assert(self._input_capture, "got mouse event after releasing capture")

   local pt = self:_get_selected_brick(mouse_pos.x, mouse_pos.y)
   if self._cursor_entity then
      -- move the  cursor, if one was specified.   
      radiant.entities.move_to(self._cursor_entity, pt or OFFSCREEN)
      -- if the user right-clicked, rotate the cursor
      if event and event:up(2) then
         self._rotation = (self._rotation + 90) % 360
         self._cursor_entity:add_component('mob'):turn_to(self._rotation)
      end
   end

   -- if the user installed a progress handler, go ahead and call it now
   if self._progress_cb then
      self._progress_cb(self, pt or OFFSCREEN, self._rotation)
   end

   -- early exit if the ray missed the entire world   
   if not pt then
      return
   end

   if event and event:up(1) then
      local finished = not self._allow_shift_queuing or not self:_shift_down()
      if self._locations_remaining_count then
         self._locations_remaining_count = self._locations_remaining_count - 1
         finished = finished and self._locations_remaining_count <= 0
      end

      if self._done_cb then
         self._done_cb(self, pt, self._rotation, finished)
      end
      if finished then
         self._input_capture:destroy()
         self._input_capture = nil

         if self._always_cb then
            self._always_cb(self)
         end
      end
   end
end

-- handles keyboard events from the input service
--
function LocationSelector:_on_keyboard_event(e)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self._input_capture:destroy()
      self._input_capture = nil

      if self._fail_cb then
         self._fail_cb(self, { error = 'cancelled via keyboard '})
      end
      if self._always_cb then
         self._always_cb(self)
      end
   end
end

-- run the location selector.   should be called after the selector
-- has been configured (cursor entity, etc.).  be sure to call
-- :destroy() when you no longer need the selection active.
--
function LocationSelector:go()
   self._rotation = 0

   -- capture the mouse.  Call our _on_mouse_event each time, passing in
   -- the entity that we're supposed to create whenever the user clicks.
   -- capture the mouse.  Call our _on_mouse_event each time, passing in
   -- the entity that we're supposed to create whenever the user clicks.
   self._input_capture = stonehearth.input:capture_input()
                           :on_mouse_event(function(e)
                                 self:_on_mouse_event(e, e)
                                 return true
                              end)
                           :on_keyboard_event(function(e)
                                 self:_on_keyboard_event(e)
                                 return true
                              end)

   -- fake an initial event to move the cursor under the mouse
   if self._progress_cb then
      self:_on_mouse_event(_radiant.client.get_mouse_position())
   end
   return self
end

return LocationSelector
