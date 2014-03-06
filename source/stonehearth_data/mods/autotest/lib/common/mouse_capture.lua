local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f

local _captures = {}
local _region_selector

local MouseEvent = class()
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

local MouseCapture = class()
function MouseCapture:destroy()
   _captures[self] = nil
end

function MouseCapture:on_input(cb)
   self.callback = cb
end

local RegionSelector = class()
function RegionSelector:__init(p0, p1)
   self._p0 = p0
   self._p1 = p1
   self._drag_end = Point3(p0.x, p0.y, p0.z)
end

function RegionSelector:destroy()
   self._progress_cb = nil
   self._done_cb = nil
   self._fail_cb = nil
end

function RegionSelector:progress(cb)
   assert(not self._progress_cb, 'autotest only supports 1 :fail() cb')
   self._progress_cb = cb
   return self
end

function RegionSelector:done(cb)
   assert(not self._done_cb, 'autotest only supports 1 :fail() cb')
   self._done_cb = cb
   return self
end

function RegionSelector:fail(cb)
   assert(not self._fail_cb, 'autotest only supports 1 :fail() cb')
   self._fail_cb = cb
   return self
end

function RegionSelector:_schedule_callback()
   local now = radiant.gamestate.now()
   radiant.set_timer(now + 20, function ()
         self:_fire_next_callback()
      end)
end

function RegionSelector:_fire_next_callback()
   self._drag_end.x = math.min(self._drag_end.x + 1, self._p1.x)
   self._drag_end.y = math.min(self._drag_end.y + 1, self._p1.y)
   self._drag_end.z = math.min(self._drag_end.z + 1, self._p1.z)
   if self._drag_end == self._p1 then
      if self._done_cb then
         self._done_cb(Cube3(self._p0, self._drag_end))
      end
   else
      if self._progress_cb then
         self._progress_cb(Cube3(self._p0, self._drag_end))
      end
      self:_schedule_callback()
   end
end

local function _capture_input_patch()
   local c = MouseCapture()
   _captures[c] = true
   return c
end

local function _select_xz_region_patch()
   if not _region_selector then
      error('server failed to push an xz region before client request')
   end
   local promise = _region_selector
   _region_selector:_schedule_callback()
   _region_selector = nil
   return promise
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

function mouse_capture.set_select_xz_region(p0, p1)
   if _region_selector then
      error('cannot have more than 1 region selector active in autotest')
   end
   _region_selector = RegionSelector(p0, p1)
end

local _radiant_client_capture_input = _radiant.client._capture_input
local _radiant_client_select_xz_region = _radiant.client.select_xz_region

function mouse_capture.hook()
   rawset(_radiant.client, 'capture_input', _capture_input_patch)
   rawset(_radiant.client, 'select_xz_region', _select_xz_region_patch)
end

return mouse_capture
