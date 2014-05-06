local log = radiant.log.create_logger('hilight_service')

local HilightService = class()

function HilightService:initialize()
   self._hilighted_entity_id = 0
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

function HilightService:get_hilighted_id()
   return self._hilighted_entity_id
end

function HilightService:_on_frame()
   local results = _radiant.client.query_scene(self._mouse_x, self._mouse_y)
   local hilghted_entity_id = 0

   if results:is_valid() then
      -- Look for a selectable object to hilight.
      local i = 0

      for i = 0, results:num_results() - 1 do
         local id = results:objectid_of(i)
         local obj = _radiant.client.get_object(id)
         local re = _radiant.client.get_render_entity(obj)

         if re and not re:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
            hilghted_entity_id = id
            break
         end
      end

   end
   if hilghted_entity_id ~= self._hilighted_entity_id then
      local last_hilghted_entity, hilghted_entity
      if hilghted_entity_id ~= 0 then
         hilghted_entity = _radiant.client.get_object(hilghted_entity_id)
      end
      if self._hilighted_entity_id ~= 0 then
         last_hilghted_entity = _radiant.client.get_object(self._hilighted_entity_id)
      end

      self._last_hilighted_entity_id = self._hilighted_entity_id
      self._hilighted_entity_id = hilghted_entity_id
      _radiant.client.hilight_entity_by_id(hilghted_entity_id)

      if last_hilghted_entity and last_hilghted_entity:is_valid() then
         radiant.events.trigger(last_hilghted_entity, 'stonehearth:hilighted_changed')
      end
      if hilghted_entity and hilghted_entity:is_valid() then
         radiant.events.trigger(hilghted_entity, 'stonehearth:hilighted_changed')
      end
   end

   return false
end

return HilightService