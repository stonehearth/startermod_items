local log = radiant.log.create_logger('selection_service')

local SelectionService = class()

function SelectionService:initialize()
   self._ui_view_mode = 'normal'
   self._selected_id = 0

   _radiant.call('stonehearth:get_ui_mode'):done(
      function (o)
         self._ui_view_mode = o.mode
      end
   )

   radiant.events.listen(radiant.events, 'stonehearth:ui_mode_changed', function(e)
      self._ui_view_mode = e.mode
   end)

   stonehearth.input:push_mouse_handler(function(e)
      return self:_on_mouse_input(e)
   end)
end

function SelectionService:get_selected_id()
   return self._selected_id
end

function SelectionService:_select_entity(id)
   local obj = _radiant.client.get_object(id)

   local tc = obj:get_component('terrain')

   if tc ~= nil then
      _radiant.client.select_entity_by_id(0)
      self._selected_id = 0
   else
      _radiant.client.select_entity_by_id(id)
      self._selected_id = id
   end
end

function SelectionService:_do_selection(results)
   if results:is_valid() then
      local i = 0

      for i = 0, results:num_results() - 1 do
         local id = results:objectid_of(i)
         local obj = _radiant.client.get_object(id)
         local re = _radiant.client.get_render_entity(obj)

         if re and not re:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
            self:_select_entity(id)
            break
         end
      end
   else
      _radiant.client.select_entity_by_id(0)
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