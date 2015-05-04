local Point3 = _radiant.csg.Point3
local Quat = _radiant.csg.Quaternion
local Ray = _radiant.csg.Ray3
local log = radiant.log.create_logger('camera')
local input_constants = require('constants').input

local gutter_size = -1
local smoothness = 0.0175
local min_height = 10

local PlayerCameraController = class()

function PlayerCameraController:initialize()
   self._sv.camera_disabled = false
   self._sv.position = Point3(0, 120, 190)
   self._sv.lookat = Point3(0, -125, -340)
   self._sv.lookat:normalize()

   self:restore()
end


function PlayerCameraController:restore()
   self._continuous_delta = Point3(0, 0, 0)
   self._impulse_delta = Point3(0, 0, 0)

   self._mouse_data = {
      x = 0,
      y = 0,
      dx = 0,
      dy = 0,
      wheel = 0,
      focused = true
   }

   self._mouse_dead_zone_x = 0
   self._mouse_dead_zone_y = 0

   self._drag_cursor = nil
   self._orbiting = false
   self._dragging = false
   self._drag_start = Point3(0, 0, 0)
   self._drag_origin = Point3(0, 0, 0)

   self._min_zoom = 10
   self._max_zoom = 300

   self._input_capture = stonehearth.input:capture_input()

   self._input_capture:on_input(function(e)
      if not self._sv.camera_disabled then
        self:_on_input(e)
      end
      -- Don't consume the event, since the UI might want to do something, too.
      return false
    end)
end


function PlayerCameraController:_get_orbit_target() 
  local r = self:_find_target()
  if r:get_result_count() > 0 then
     return r:get_result(0).intersection
  else
     local forward_dir = _radiant.renderer.camera.get_forward()

     if math.abs(forward_dir.y) < 0.0001 then
        return nil
     end

     local p = stonehearth.camera:get_position()
     -- Pick a location at the user's cursor, 20 units above the '0' level.
     local d = 20 - p.y / forward_dir.y
     forward_dir:scale(d)

     return forward_dir + p
  end
end

function PlayerCameraController:_calculate_keyboard_orbit()

  if _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_Q) or
    _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_E) then

     local deg_x = 0;
     local deg_y = 0;
     
     if _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_Q) then
        deg_x = -3
     else
        deg_x = 3
     end

     local orbit_target = self:_get_orbit_target()

     if orbit_target then
        self:_orbit(orbit_target, deg_y, deg_x, 30.0, 70.0)
     end
  end
end

function PlayerCameraController:_calculate_orbit(e)
   if not self._mouse_in_dead_zone and _radiant.client.is_mouse_button_down(_radiant.client.MouseInput.MOUSE_BUTTON_2) then

      local orbit_target = self:_get_orbit_target()
      local deg_x = e.dx / -3.0
      local deg_y = e.dy / -2.0

      if orbit_target then
         self:_orbit(orbit_target, deg_y, deg_x, 30.0, 70.0)
      end
   end
end

function PlayerCameraController:_orbit(target, x_deg, y_deg, min_x, max_x)
  local deg_to_rad = 3.14159 / 180.0

  local origin_vec = stonehearth.camera:get_position() - target

  if origin_vec:length() < 0.1 then
    return
  end

  local new_position = Point3(origin_vec.x, origin_vec.y, origin_vec.z)
  origin_vec:normalize()

  local x_ang = math.acos(origin_vec.y) / deg_to_rad

  if x_ang + x_deg > max_x then
    x_deg = max_x - x_ang
  elseif x_ang + x_deg < min_x then
    x_deg = min_x - x_ang
  end

  local left = stonehearth.camera:get_left()
  local y_rot = Quat(Point3(0, 1, 0), y_deg * deg_to_rad)
  local x_rot = Quat(left, x_deg * deg_to_rad)

  new_position = x_rot:rotate(new_position)
  new_position = y_rot:rotate(new_position)

  -- We take the orbiter to have a higher 'priority' than scrolling, so
  -- don't scroll when we orbit.
  local position = new_position + target
  self._continuous_delta = Point3(0, 0, 0)

  -- We call the camera-service directly here to set position, because we want to
  -- snap to the new position; no interpolation whatsoever--just go there!
  stonehearth.camera:set_position(position)
  stonehearth.camera:look_at(target)

  radiant.events.trigger_async(stonehearth.camera, 'stonehearth:camera:update', {
    pan = false,
    orbit = true,
    zoom = false,
  })  
end

function PlayerCameraController:_on_input(e) 
    if e.type == _radiant.client.Input.MOUSE then
      self:_accumulate_mouse_input(e.mouse, e.focused)
    elseif e.type == _radiant.client.Input.KEYBOARD then
      self:_on_keyboard_input(e.keyboard)
    end
end

function PlayerCameraController:_on_keyboard_input(e)
  local drag_key_down = _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_SPACE)
  
  if drag_key_down and not self._drag_cursor then
    self._drag_cursor = _radiant.client.set_cursor('stonehearth:cursors:camera_pan')
  elseif not drag_key_down and self._drag_cursor then
    self._drag_cursor:destroy()
    self._drag_cursor = nil
  end
end

function PlayerCameraController:_accumulate_mouse_input(e, focused)
   self._mouse_data.x = e.x
   self._mouse_data.y = e.y
   self._mouse_data.dx = self._mouse_data.dx + e.dx
   self._mouse_data.dy = self._mouse_data.dy + e.dy
   self._mouse_data.wheel = self._mouse_data.wheel + e.wheel
   self._mouse_data.focused = focused
end

function PlayerCameraController:_process_mouse()
   self:_on_mouse_input(self._mouse_data, self._mouse_data.focused)

   self._mouse_data.dx = 0
   self._mouse_data.dy = 0
   self._mouse_data.wheel = 0
end

function PlayerCameraController:_on_mouse_input(e, focused)
  self:_calculate_mouse_dead_zone(e)
  self:_calculate_drag(e)
  self:_calculate_orbit(e)
  self:_calculate_zoom(e)
end

function PlayerCameraController:_calculate_mouse_dead_zone(e)

   if _radiant.client.is_mouse_button_down(_radiant.client.MouseInput.MOUSE_BUTTON_2) then
     self._mouse_dead_zone_x = self._mouse_dead_zone_x + math.abs(e.dx)
     self._mouse_dead_zone_y = self._mouse_dead_zone_y + math.abs(e.dy)
   else 
     self._mouse_dead_zone_x = 0
     self._mouse_dead_zone_y = 0
   end

   if self._mouse_dead_zone_x < input_constants.mouse.dead_zone_size or self._mouse_dead_zone_y < input_constants.mouse.dead_zone_size then
      self._mouse_in_dead_zone = true
   else 
      self._mouse_in_dead_zone = false
   end
end

function PlayerCameraController:_calculate_zoom(e)
  local zoomInKeyDown = _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_SEMICOLON)
  local zoomOutKeyDown = _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_EQUAL)

  if not zoomInKeyDown and not zoomOutKeyDown and e.wheel == 0 then
     return
  end

  local query = _radiant.renderer.scene.cast_screen_ray(e.x, e.y)
  if query:get_result_count() == 0 then
    return
  end

  local target = query:get_result(0).intersection
  local pos = self._sv.position

  local distance_to_target = pos:distance_to(target)

  if ((e.wheel > 0 or zoomInKeyDown) and distance_to_target <= self._min_zoom) or
     ((e.wheel < 0 or zoomOutKeyDown) and distance_to_target >= self._max_zoom) then
    return
  end

  local short_factor = 0.2
  local med_factor = 0.3
  local far_factor = 0.4
  local factor = far_factor

  if distance_to_target < 100.0 then
    factor = short_factor
  elseif distance_to_target < 500.0 then
    factor = med_factor
  end

  local distance_to_cover = distance_to_target * factor * math.abs(e.wheel)

  if e.wheel > 0 or zoomInKeyDown then
    -- Moving towards the target.
    if distance_to_target - distance_to_cover < self._min_zoom then
      distance_to_cover = distance_to_target - self._min_zoom
    end
  else
    -- Moving away from the target.
    if distance_to_cover + distance_to_target > self._max_zoom then
      distance_to_cover = self._max_zoom - distance_to_target
    end
  end

  local sign = 1
  if e.wheel > 0 or zoomInKeyDown then
    sign = -1
  end

  distance_to_cover = distance_to_cover * sign

  local dir = pos - target
  dir:normalize()
  dir:scale(distance_to_cover)

  self._impulse_delta = dir

  radiant.events.trigger_async(stonehearth.camera, 'stonehearth:camera:update', {
      pan = false,
      orbit = false,
      zoom = true,
    })

end

function PlayerCameraController:_find_target()
  local forward_dir = stonehearth.camera:get_forward()
  forward_dir:scale(10000.0)

  local p = stonehearth.camera:get_position()

  return _radiant.renderer.scene.cast_ray(p, forward_dir)
end

function PlayerCameraController:_calculate_drag(e)
  local drag_key_down = _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_SPACE)
  
  if drag_key_down and not self._dragging then
    local r = _radiant.renderer.scene.cast_screen_ray(e.x, e.y)
    local screen_ray = _radiant.renderer.scene.get_screen_ray(e.x, e.y)
    self._dragging = true
    self._drag_origin = screen_ray.origin

    if r:get_result_count() > 0  then
      -- avoid the "stuck in trees" issue for now by stabbing through to the terrain.
      -- it might be more correct to stab through to the first front-face instead,
      -- but we don't have the infractrsture for that just yet.
      for result in r:each_result() do
         if result.entity and result.entity:get_id() == 1 then
            self._drag_start = result.intersection
            break
         end
      end
      -- still nothing?  use the first result and hope for the best!
      if not self._drag_start then
         self._drag_start = r:get_result(0).intersection
      end
    else
      local d = -self._drag_origin.y / screen_ray.direction.y
      screen_ray.direction:scale(d)
      self._drag_start = screen_ray.origin + screen_ray.direction
    end

    local root = radiant.entities.get_entity(1)
    local terrain_comp = root:get_component('terrain')
    local bounds = terrain_comp:get_bounds()
    if not bounds:contains_inclusive(self._drag_start) then
      self._dragging = false
    end
  elseif not drag_key_down and self._dragging then
    self._dragging = false
  end

  if not self._dragging then
    return
  end

  self:_drag(e.x, e.y)
end

function PlayerCameraController:_process_keys()
  self:_calculate_keyboard_orbit()
  self:_calculate_keyboard_pan()
end

function PlayerCameraController:_calculate_keyboard_pan()
  local left = Point3(0, 0, 0)
  local forward = Point3(0, 0, 0)
  local x_scale = 0
  local y_scale = 0
  local speed = stonehearth.camera:get_position().y -- surprisingly this feels good with no modifiers!
  
  if _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_A) or 
     _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_LEFT) then
     x_scale = -speed
  elseif _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_D) or
         _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_RIGHT) then
     x_scale = speed
  end

  left = stonehearth.camera:get_left()
  left:scale(x_scale)

  if _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_W) or 
     _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_UP) then
     y_scale = speed
  elseif _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_S) or
         _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_DOWN) then
     y_scale = -speed
  end

  forward = stonehearth.camera:get_forward()
  forward.y = 0
  forward:normalize()
  forward:scale(y_scale)

  self._continuous_delta = forward + left

  if x_scale ~= 0 or y_scale ~= 0 then
    radiant.events.trigger_async(stonehearth.camera, 'stonehearth:camera:update', {
        pan = true,
        orbit = false,
        zoom = false,
      })
  end

end


function PlayerCameraController:_drag(x, y)
  local screen_ray = _radiant.renderer.scene.get_screen_ray(x, y)

  if screen_ray.direction.y == 0 then
    return
  end

  -- Intersect the plane, facing up (y = 1), on the point defined by _drag_start.
  local d = (self._drag_start.y - self._drag_origin.y) / screen_ray.direction.y

  if d < 0 then
    return
  end
  
  screen_ray.direction:scale(d)

  local drag_end = screen_ray.direction + self._drag_origin

  local drag_delta = (self._drag_start - drag_end)
  local next_pos = self._drag_origin + drag_delta
  local proj_pos = Point3(self._sv.position.x, 0.0, self._sv.position.z)
  if drag_end:distance_to(proj_pos) < 900 then
    self._sv.position = next_pos
  end
  if self._drag_start ~= drag_end then
    radiant.events.trigger_async(stonehearth.camera, 'stonehearth:camera:update', {
        pan = true,
        orbit = false,
        zoom = false,
      })
  end
end


function PlayerCameraController:enable_camera(enabled)
   self._sv.camera_disabled = not enabled
end


function PlayerCameraController:set_position(pos)
   self._sv.position = pos
end

function PlayerCameraController:look_at(where)
   self._sv.lookat = where - self._sv.position
   self._sv.lookat:normalize()
end


function PlayerCameraController:update(frame_time)
   if stonehearth.input:ui_has_capture() then
      self._impulse_delta = Point3(0, 0, 0)
      self._continuous_delta = Point3(0, 0, 0)
   else
      self:_process_mouse()
      self:_process_keys()
   end

   local scaled_continuous_delta = Point3(self._continuous_delta)
   scaled_continuous_delta:scale(frame_time / 1000.0)

   self._sv.position = self._sv.position + (scaled_continuous_delta) + self._impulse_delta
   self.__saved_variables:mark_changed()

   self._impulse_delta = Point3(0, 0, 0)

   local lerp_pos = stonehearth.camera:get_position():lerp(self._sv.position, smoothness * frame_time)
   local rot = Quat()
   rot:look_at(Point3(0, 0, 0), self._sv.lookat)
   rot:normalize()

   return lerp_pos, rot
end

return PlayerCameraController
