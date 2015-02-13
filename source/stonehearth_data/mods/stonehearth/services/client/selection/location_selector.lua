local selector_util = require 'services.client.selection.selector_util'
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

function LocationSelector:_call_once(name, ...)
   local method_name = '_' .. name .. '_cb'
   if self[method_name] then
      local method = self[method_name]
      self[method_name] = nil
      method(self, ...)
   end
end

function LocationSelector:resolve(...)
   self:_call_once('done', ...)
   self:_call_once('always')
   self:_cleanup()
   return self
end

function LocationSelector:reject(...)
   self:_call_once('fail', ...)
   self:_call_once('always')
   self:_cleanup()
   return self
end

function LocationSelector:notify(...)
   if self._progress_cb then
      self._progress_cb(self, ...)
   end
   return self
end

function LocationSelector:_cleanup()
   stonehearth.selection:register_tool(self, false)

   self._fail_cb = nil
   self._progress_cb = nil
   self._done_cb = nil
   self._always_cb = nil

   if self._cursor_obj then
      self._cursor_obj:destroy()
      self._cursor_obj = nil
   end

   if self._invalid_cursor then
      self._invalid_cursor:destroy()
      self._invalid_cursor = nil
   end

   if self._cursor_entity then
      radiant.entities.destroy_entity(self._cursor_entity)
      self._cursor_entity = nil
   end 

   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end
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

function LocationSelector:set_cursor(cursor)
   self._cursor = cursor
   return self
end

function LocationSelector:get_cursor_entity()
   return self._cursor_entity
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

function LocationSelector:set_rotation_disabled(disabled)
   self._rotation_disabled = disabled
   return self
end

function LocationSelector:get_rotation()
   return self._rotation
end

function LocationSelector:set_rotation(rotation)   
   self._rotation = rotation
   if self._cursor_entity then
      self._cursor_entity:add_component('mob'):turn_to(self._rotation)
   end

   -- if the user installed a progress handler, go ahead and call it now
   self:notify(self._pt, self._rotation)

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
   self:reject({ error = 'selector destroyed'})
end

-- given the x, y coordinate on screen, return the brick that is a
-- candidate for selection.  trace a ray through that point in the screen,
-- ignoring nodes between ourselves and the desired brick (e.g. the cursor
-- is almost certainly going to get in the way!)
--
function LocationSelector:_get_selected_brick(x, y)
   return selector_util.get_selected_brick(x, y, function(result)
         -- select the brick in front...
         result.brick = result.brick + result.normal

         -- skip the cursor...
         if result.entity == self._cursor_entity then
            return stonehearth.selection.FILTER_IGNORE
         end
         
         -- if not filter is installed, return the first brick
         if not self._filter_fn then
            return true
         end
         return self._filter_fn(result, self)
      end)
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

   if stonehearth.selection.user_cancelled(event) then
      self._input_capture:destroy()
      self._input_capture = nil

      self:reject({ error = 'selection cancelled '})
      return
   end

   local pt = self:_get_selected_brick(mouse_pos.x, mouse_pos.y)
   self._pt = pt
   if self._cursor_entity then
      -- move the  cursor, if one was specified.   
      radiant.entities.move_to(self._cursor_entity, pt or OFFSCREEN)      
   end

   -- if the user installed a progress handler, go ahead and call it now
   self:notify(self._pt, self._rotation)

   -- early exit if the ray missed the entire world   
   if not pt then
      -- Show an invalid icon for the cursor, if we aren't already.
      if not self._invalid_cursor then
         self._invalid_cursor = _radiant.client.set_cursor('stonehearth:cursors:invalid_hover')
      end
      return
   end

   -- At this point, the location is valid, so clean up any invalid cursor we might
   -- be showing.
   if self._invalid_cursor then
      self._invalid_cursor:destroy()
      self._invalid_cursor = nil
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
         self:_call_once('always')
         self:_cleanup()
      end
   end
end

-- handles keyboard events from the input service
--
function LocationSelector:_on_keyboard_event(e)
   local deltaRot = 0

   -- period and comma rotate the cursor
   if not self._rotation_disabled then
      if e.key == _radiant.client.KeyboardInput.KEY_COMMA and e.down then
         deltaRot = 90
      elseif e.key == _radiant.client.KeyboardInput.KEY_PERIOD and e.down then
         deltaRot = -90
      end

      if deltaRot ~= 0 then
         local new_rotation = (self._rotation + deltaRot) % 360
         self:set_rotation(new_rotation)

         -- remember the last rotation for the next time we bring up another
         -- LocationSelector.
         LocationSelector.last_rotation = new_rotation
      end
   end
end

-- run the location selector.   should be called after the selector
-- has been configured (cursor entity, etc.).  be sure to call
-- :destroy() when you no longer need the selection active.
--
function LocationSelector:go()
   -- install a new mouse cursor if requested by the client.  this cursor
   -- will stick around until :destroy() is called on the selector!
   if self._cursor then
      self._cursor_obj = _radiant.client.set_cursor(self._cursor)
   end
   
   self._invalid_cursor = nil
   stonehearth.selection:register_tool(self, true)

   if self._rotation == nil then
      self:set_rotation(LocationSelector.last_rotation or 0)
   end

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
