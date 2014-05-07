local log = radiant.log.create_logger('selection_service')

local SelectionService = class()

function SelectionService:initialize()
   self._selected_id = 0

   self._input_capture = stonehearth.input:capture_input()
                              :on_mouse_event(function(e)
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
      local entity = radiant.entities.get_entity(id)
      local player_id = radiant.entities.get_player_id(entity)
      --if  player_id == 'player_1' or player_id == '' then
         _radiant.client.select_entity_by_id(id)
         self._selected_id = id
      --[[else
         _radiant.client.select_entity_by_id(0)
         self._selected_id = 0
      end]]
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