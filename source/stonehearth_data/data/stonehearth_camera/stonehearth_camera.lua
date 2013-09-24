local Vec3 = _radiant.csg.Point3f
local StonehearthCamera = class()

function StonehearthCamera:__init()
  self._input_capture = _radiant.client.capture_input()

  self._input_capture:on_input(function(e)
      if e.type == _radiant.client.Input.MOUSE then
          self:_on_mouse_event(e.mouse, 1920, 1080, 20)
          return true
      end
      return false
    end)
end

function StonehearthCamera:_on_mouse_event(e, screen_x, screen_y, gutter_size)
  local mouse_dx = e.dx / 10.0
  local mouse_dy = e.dy / 10.0

  local mouse_x = e.x
  local mouse_y = e.y

  if mouse_dx ~= 0 or mouse_dy ~= 0 then
    local left = Vec3(0, 0, 0)
    local forward = Vec3(0, 0, 0)

    if mouse_x < gutter_size or mouse_x > screen_x - gutter_size then
      left = _radiant.renderer.camera.get_left()
      left:scale(mouse_dx)
    end

    if mouse_y < gutter_size or mouse_y > screen_y - gutter_size then
      forward = _radiant.renderer.camera.get_forward()
      forward.y = 0
      forward:normalize()
      forward:scale(mouse_dy)
    end

    local delta = forward + left

    _radiant.renderer.camera.translate(delta)
  end
end

function StonehearthCamera:look_at(point)
end

StonehearthCamera:__init()
return StonehearthCamera