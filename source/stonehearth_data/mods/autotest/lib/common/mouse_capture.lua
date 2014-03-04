local Point3f = _radiant.csg.Point3f

local _captures = {}

local MouseEvent = class()
local MouseCapture = class()

function MouseEvent:__init(o)
   self.type = _radiant.client.Input.MOUSE
   self.mouse = {
      x = o.x or 0,
      y = o.y or 0,
      wheel = o.wheel or 0,
      up = function(_, idx) return self:_check_button(idx, o.up) end,
      down = function(_, idx) return self:_check_button(idx, o.down) end,
   }
   self._options = o
end

function MouseEvent:_check_button(idx, buttons)
   if buttons then
      for _, button in ipairs(buttons) do
         if button == idx then
            return true
         end
      end
   end
end

function MouseEvent:down(idx)
   for _, button in ipairs(self._options.down or {}) do
      if button == idx then
         return true
      end
   end
end

function MouseCapture:destroy()
   _captures[self] = nil
end

function MouseCapture:on_input(cb)
   self.callback = cb
end

local function capture_input_patch()
   local c = MouseCapture()
   _captures[c] = true
   return c
end

local function send_event(e)
   for cap, _ in pairs(_captures) do
      if cap.callback then
         cap.callback(e)
      end
   end
end

local mouse_capture = {}

function mouse_capture.click(x, y, z)
   local pt = _radiant.renderer.camera.world_to_screen(Point3f(x, y, z))
   send_event(MouseEvent({ x = pt.x, y = pt.y, down = { 1 } }))
   send_event(MouseEvent({ x = pt.x, y = pt.y, up = { 1 } }))
end

local _radiant_client_capture_input = _radiant.client._capture_input
function mouse_capture.hook()
   rawset(_radiant.client, 'capture_input', capture_input_patch)
end

return mouse_capture
