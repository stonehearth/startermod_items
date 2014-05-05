local log = radiant.log.create_logger('selection_service')

local SelectionService = class()

function SelectionService:initialize()
   self._input_capture = stonehearth.input:capture_input()
                              :on_mouse_event(function(e)
                                   return self:_on_mouse_input(e)
                                 end)
   self._selected_id = 0
end

function SelectionService:get_selected_id()
   return self._selected_id
end

function SelectionService:get_selected_entity()
   return self._selected_id > 0 and _radiant.client.get_object(self._selected_id) or nil
end

function SelectionService:select_entity(arg1)
   local selected_id = type(arg1) == 'number' and arg1 or arg1:get_id()

   local selected_entity = selected_id > 0 and _radiant.client.get_object(selected_id) or nil
   if not selected_entity or selected_entity:get_component('terrain') then
      selected_entity = nil
      selected_id = 0
   end

   if selected_id == self._selected_id then
      return
   end

   local selected, last_selected
   if selected_id ~= 0 then
      selected = _radiant.client.get_object(selected_id)
   end
   if self._selected_id ~= 0 then
      last_selected = _radiant.client.get_object(self._selected_id)
   end

   self._selected_id = selected_id
   _radiant.client.select_entity_by_id(selected_id)

   if last_selected and last_selected:is_valid() then
      radiant.events.trigger(last_selected, 'stonehearth:selection_changed')
   end
   if selected and selected:is_valid() then
      radiant.events.trigger(selected, 'stonehearth:selection_changed')
   end
   radiant.events.trigger(radiant, 'stonehearth:selection_changed')
end

function SelectionService:_do_selection(results)
   if results:is_valid() then
      local i = 0

      for i = 0, results:num_results() - 1 do
         local id = results:objectid_of(i)
         local obj = _radiant.client.get_object(id)
         local re = _radiant.client.get_render_entity(obj)

         if re and not re:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
            self:select_entity(id)
            break
         end
      end
   else
      self:select_entity(0)
   end
end

function SelectionService:_on_mouse_input(e)
   if e:up(1) then
      self:_do_selection(_radiant.client.query_scene(e.x, e.y))
      return true
   end

   return false
end

return SelectionService