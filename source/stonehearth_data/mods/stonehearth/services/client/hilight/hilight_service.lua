local log = radiant.log.create_logger('hilight_service')

local HilightService = class()

function HilightService:initialize()
   self._mouse_x = 0
   self._mouse_y = 0

   self._frame_trace = _radiant.client.trace_render_frame()
   self._frame_trace:on_frame_start('update hilight service', function(now, alpha, frame_time)
      return self:_on_frame()
   end)

   self._input_capture = stonehearth.input:capture_input()
                              :on_mouse_event(function(e)
                                    self._mouse_x = e.x
                                    self._mouse_y = e.y
                                    return false
                                 end)
end

function HilightService:get_hilighted()
   return self._hilighted
end

function HilightService:_on_frame()
   local hilighted
   local results = _radiant.client.query_scene(self._mouse_x, self._mouse_y)

   for r in results:each_result() do 
      local re = _radiant.client.get_render_entity(r.entity)
      if re and not re:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
         hilighted = r.entity
         break
      end
   end

   if hilighted ~= self._hilighted then
      local last_hilghted = self._hilighted
      self._hilighted = hilighted

      _radiant.client.hilight_entity(self._hilighted)

      if last_hilghted and last_hilghted:is_valid() then
         radiant.events.trigger(last_hilghted, 'stonehearth:hilighted_changed')
      end
      if self._hilighted and self._hilighted:is_valid() then
         radiant.events.trigger(self._hilighted, 'stonehearth:hilighted_changed')
      end
   end

   return false
end

return HilightService