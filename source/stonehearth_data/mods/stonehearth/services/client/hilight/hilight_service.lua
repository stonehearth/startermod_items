local log = radiant.log.create_logger('hilight_service')

local HilightService = class()

function HilightService:initialize()
   self._hilghted_entity_id = 0
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

function HilightService:_on_frame()
   local results = _radiant.client.query_scene(self._mouse_x, self._mouse_y)

   -- Clear existing hilight.
   _radiant.client.hilight_entity_by_id(0)

   if results:is_valid() then
      -- Look for a selectable object to hilight.
      local i = 0

      for i = 0, results:num_results() - 1 do
         local id = results:objectid_of(i)
         local obj = _radiant.client.get_object(id)
         local re = _radiant.client.get_render_entity(obj)

         if re and not re:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
            _radiant.client.hilight_entity_by_id(id)
            break
         end
      end

   end

   return false
end

return HilightService