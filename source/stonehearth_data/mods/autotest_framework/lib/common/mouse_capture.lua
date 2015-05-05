local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local _mouse_queue = {}
local _mouse_queue_timer_set

local MouseEvent = class()
function MouseEvent:__init(o)
   self.type = _radiant.client.Input.MOUSE
   self.mouse = {
      x = o.x or 0,
      y = o.y or 0,
      dx = 0,
      dy = 0,
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

--

local RegionSelectorDeferred = class()

function RegionSelectorDeferred:destroy()
   self._client_attached = false
   self._server_attached = false
   self._progress_cb = nil
   self._done_cb = nil
   self._fail_cb = nil
   self._always_cb = nil
   self._p0 = nil
   self._p1 = nil
end

function RegionSelectorDeferred:is_active()
   return self._client_attached and self._server_attached
end

function RegionSelectorDeferred:notify_client_attached()
   self._client_attached = true
   self:_schedule_callback()
end

function RegionSelectorDeferred:set_region(p0, p1)
   self._server_attached = true
   self._p0 = p0
   self._p1 = p1
   self._drag_end = Point3(p0.x, p0.y, p0.z)
   self:_schedule_callback()
end

function RegionSelectorDeferred:progress(cb)
   assert(not self._progress_cb, 'autotest only supports 1 :progress() cb')
   self._progress_cb = cb
   return self
end

function RegionSelectorDeferred:done(cb)
   assert(not self._done_cb, 'autotest only supports 1 :done() cb')
   self._done_cb = cb
   return self
end

function RegionSelectorDeferred:fail(cb)
   assert(not self._fail_cb, 'autotest only supports 1 :fail() cb')
   self._fail_cb = cb
   return self
end

function RegionSelectorDeferred:always(cb)
   assert(not self._always_cb, 'autotest only supports 1 :always() cb')
   self._always_cb = cb
   return self
end

function RegionSelectorDeferred:_schedule_callback()
   if self._client_attached and self._server_attached then
      radiant.set_realtime_timer(20, function ()
            self:_fire_next_callback()
         end)
   end
end

function RegionSelectorDeferred:_fire_next_callback()
   self._drag_end.x = math.min(self._drag_end.x + 1, self._p1.x)
   self._drag_end.y = math.min(self._drag_end.y + 1, self._p1.y)
   self._drag_end.z = math.min(self._drag_end.z + 1, self._p1.z)
   if self._drag_end == self._p1 then
      if self._done_cb then
         self._done_cb(Cube3(self._p0, self._drag_end))
      end

      if self._always_cb then
         self._always_cb(Cube3(self._p0, self._drag_end))
      end
   else
      if self._progress_cb then
         self._progress_cb(Cube3(self._p0, self._drag_end))
      end
      self:_schedule_callback()
   end
end

local function start_mouse_queue_timer()
   if not _mouse_queue_timer_set then
      _mouse_queue_timer_set = true
      radiant.set_realtime_timer(10, function()
            _mouse_queue_timer_set = false
            if #_mouse_queue > 0 then
               local event = table.remove(_mouse_queue, 1)
               stonehearth.input:_dispatch_input(event)
               start_mouse_queue_timer()
            end
         end)
   end
end

local function queue_mouse_event(e)
   table.insert(_mouse_queue, e)
   start_mouse_queue_timer();
end

local _region_selector_deferred = RegionSelectorDeferred()
local function _create_xz_region_selector_proxy()
   assert(not _region_selector_deferred:is_active())

   _region_selector_deferred:notify_client_attached()
   return _region_selector_deferred
end

--

RegionSelector = class()

function RegionSelector:require_supported(supported)
   -- nop
   return self
end

function RegionSelector:require_unblocked(unblocked)
   -- nop
   return self
end

function RegionSelector:activate()
   assert(not _region_selector_deferred:is_active())

   _region_selector_deferred:notify_client_attached()
   return _region_selector_deferred
end

--

local mouse_capture = {}

function mouse_capture.click(x, y, z)
   local pt = _radiant.renderer.camera.world_to_screen(Point3(x, y, z))
   queue_mouse_event(MouseEvent({ x = pt.x, y = pt.y, down = { 1 } }))
   queue_mouse_event(MouseEvent({ x = pt.x, y = pt.y, up = { 1 } }))
end

function mouse_capture.set_select_xz_region(p0, p1)
   --assert(not _region_selector_deferred:is_active(), 'cannot have more than 1 region selector active in autotest')
   --_region_selector_deferred:set_region(p0, p1)
   local start  = _radiant.renderer.camera.world_to_screen(p0)
   local finish = _radiant.renderer.camera.world_to_screen(p1)
   queue_mouse_event(MouseEvent({ x = start.x,  y = start.y,  down = { 1 } }))
   for i = 1,8 do
      queue_mouse_event(MouseEvent({ x = finish.x, y = finish.y }))
   end
   queue_mouse_event(MouseEvent({ x = finish.x, y = finish.y, up = { 1 } }))   
end

function mouse_capture.hook()
   start_mouse_queue_timer()
   rawset(_radiant.client, 'create_xz_region_selector', function()
      return RegionSelector()
   end)
end

return mouse_capture
