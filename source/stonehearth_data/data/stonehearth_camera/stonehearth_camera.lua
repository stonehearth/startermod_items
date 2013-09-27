local Vec3 = _radiant.csg.Point3f
local Quat = _radiant.csg.Quaternion
local Ray = _radiant.csg.Ray3

local gutter_size = 60
local scroll_speed = 1
local smoothness = 13.0
local min_height = 10

local StonehearthCamera = class()

function StonehearthCamera:__init()
  self:set_position(Vec3(0, 1, 0))
  self:look_at(Vec3(0, 0, -1))

  self:set_position(Vec3(50, 200, 150))

  self._next_position = self:get_position()
  self._scroll_delta = Vec3(0, 0, 0)
  self._zoom_delta = Vec3(0, 0, 0)
  self._orbiting = false
  self._orbit_target = Vec3(0, 0, 0)

  self:_update_camera(0)

  self._input_capture = _radiant.client.capture_input()
  self._frame_trace = _radiant.client.trace_render_frame()

  self._frame_trace:on_frame(function(frame_time)
      self:_update_camera(frame_time)
      return true
    end)

  self._input_capture:on_input(function(e)
      if e.type == _radiant.client.Input.MOUSE then
          self:_on_mouse_event(
            e.mouse, 
            _radiant.renderer.screen.get_width(), 
            _radiant.renderer.screen.get_height(), 
            gutter_size)
      end
      -- Don't consume the event, since the UI might want to do something, too.
      return false
    end)
end

function StonehearthCamera:_orbit(target, x_deg, y_deg, min_x, max_x)
  local deg_to_rad = 3.14159 / 180.0

  local origin_vec = self:get_position() - target
  local new_position = Vec3(origin_vec.x, origin_vec.y, origin_vec.z)
  origin_vec:normalize()

  local x_ang = math.acos(origin_vec.y) / deg_to_rad

  if x_ang + x_deg > max_x then
    x_deg = max_x - x_ang
  elseif x_ang + x_deg < min_x then
    x_deg = min_x - x_ang
  end

  local left = _radiant.renderer.camera.get_left()
  local y_rot = Quat(Vec3(0, 1, 0), y_deg * deg_to_rad)
  local x_rot = Quat(left, x_deg * deg_to_rad)

  new_position = x_rot:rotate(new_position)
  new_position = y_rot:rotate(new_position)

  -- We take the orbiter to have a higher 'priority' than scrolling, so
  -- don't scroll when we orbit.
  self._next_position = new_position + target
  self._scroll_delta = Vec3(0, 0, 0)

  _radiant.renderer.camera.set_position(new_position + target)
  _radiant.renderer.camera.look_at(target)
end

function StonehearthCamera:_on_mouse_event(e, screen_x, screen_y, gutter)
  self:_calculate_scroll(e, screen_x, screen_y, gutter)
  --self:_calculate_jump(e)
  self:_calculate_orbit(e)
  self:_calculate_zoom(e)
  --self:_calculate_height(e)
  -- Temporary!
  --self:_update_camera()
end

function StonehearthCamera:_calculate_zoom(e)
  if e.wheel == 0 then
    self._zoom_delta = Vec3(0, 0, 0)
    return
  end

  local query = self:_find_target_from_next_position()
  if not query.is_valid then
    return
  end

  local target = query.point
  local pos = self._next_position

  local dist = pos:distance_to(target)

  local _td = dist

  local short_factor = 0.2
  local med_factor = 0.3
  local far_factor = 0.4
  local factor = far_factor

  if dist < 100.0 then
    factor = short_factor
  elseif dist < 500.0 then
    factor = med_factor
  end

  dist = dist * factor * -e.wheel

  local dir = pos - target
  dir:normalize()
  dir:scale(dist)

  self._zoom_delta = dir

  radiant.log.info('zooming: %d', e.wheel)
end

function StonehearthCamera:_find_target()
  local forward_dir = _radiant.renderer.camera.get_forward()
  forward_dir:scale(10000.0)
  --Huh?  Why!?
  forward_dir.z = -forward_dir.z
  forward_dir.y = -forward_dir.y
  forward_dir.x = -forward_dir.x

  local p = self:get_position()

  return _radiant.renderer.scene.cast_ray(p, forward_dir)
end

function StonehearthCamera:_find_target_from_next_position()
  local forward_dir = _radiant.renderer.camera.get_forward()
  forward_dir:scale(10000.0)
  --Huh?  Why!?
  forward_dir.z = -forward_dir.z
  forward_dir.y = -forward_dir.y
  forward_dir.x = -forward_dir.x
  return _radiant.renderer.scene.cast_ray(self._next_position, forward_dir)
end

function StonehearthCamera:_calculate_orbit(e)
  if e:down(2) then
    local r = self:_find_target()--_radiant.renderer.scene.cast_screen_ray(e.x, e.y)
    if r.is_valid then
      self._orbiting = true
      self._orbit_target = r.point
      --[[local r2 = self:_find_target()
      if r2.is_valid then
        self._scroll_delta = r.point - r2.point
      end]]
    else
      return
    end
  elseif e:up(2) and self._orbiting then
    self._orbiting = false
  end

  if not self._orbiting then
    return
  end

  local deg_x = e.dx / -3.0
  local deg_y = e.dy / -2.0

  self:_orbit(self._orbit_target, deg_y, deg_x, 30.0, 70.0)
end

function StonehearthCamera:_calculate_height(e)
  local ray_dir = self:get_position() - self._target
  ray_dir.normalize()

  local r = _radiant.renderer.scene.cast_ray(self._position, ray_dir)
  if r.is_valid then
    self._next_position = r.point + Vec3(0, 30, 0)
  end
end

function StonehearthCamera:_calculate_jump(e)
  if e:up(3) then
    local r = _radiant.renderer.scene.cast_screen_ray(e.x, e.y)
    if r.is_valid then
      self._next_position = r.point + Vec3(0, 30, 0)
    end
  end
end

function StonehearthCamera:_calculate_scroll(e, screen_x, screen_y, gutter)
  if not e.in_client_area then
    self._scroll_delta = Vec3(0, 0, 0)
    return
  end

  local mouse_x = e.x
  local mouse_y = e.y

  local left = Vec3(0, 0, 0)
  local forward = Vec3(0, 0, 0)
  local x_scale = 0
  local y_scale = 0

  if mouse_x < gutter then
    x_scale = -scroll_speed
  elseif mouse_x > screen_x - gutter then
    x_scale = scroll_speed
  end
  left = _radiant.renderer.camera.get_left()
  left:scale(x_scale)

  if mouse_y < gutter then
    y_scale = -scroll_speed
  elseif mouse_y > screen_y - gutter then
    y_scale = scroll_speed
  end
  forward = _radiant.renderer.camera.get_forward()
  forward.y = 0
  forward:normalize()
  forward:scale(y_scale)

  self._scroll_delta = forward + left
end

function StonehearthCamera:_update_camera(frame_time)
  self._next_position = self._next_position + self._scroll_delta + self._zoom_delta

  -- Clear the zoom_delta, because it is an impulse and cannot clear itself.
  self._zoom_delta = Vec3(0, 0, 0)

  local interpolated_pos = _radiant.csg.lerp(self:get_position(), self._next_position, smoothness * frame_time)
  _radiant.renderer.camera.set_position(interpolated_pos)
end

function StonehearthCamera:get_position()
  return _radiant.renderer.camera.get_position()
end

function StonehearthCamera:set_position(new_pos)
  return _radiant.renderer.camera.set_position(new_pos)
end

function StonehearthCamera:look_at(where)
  _radiant.renderer.camera.look_at(where)
end

StonehearthCamera:__init()
return StonehearthCamera