local PartyRenderer = require 'services.client.party_editor.party_renderer'
local PartyEditorService = class()

function PartyEditorService:initialize()
   self._party_renderers = {}
end

function PartyEditorService:set_attack_order_command(session, response, party)
   local cursor_entity = radiant.entities.create_entity('stonehearth:attack_order_banner')

   cursor_entity:add_component('render_info')
                     :set_material('materials/ghost_item.xml')

   stonehearth.selection:select_location()
      :set_rotation_disabled(false)
      :set_cursor_entity(cursor_entity)
      :set_filter_fn(function (result, selector)
            local normal = result.normal:to_int()
            local location = result.brick:to_int()

            if normal.y ~= 1 then
               return stonehearth.selection.FILTER_IGNORE
            end
            if not radiant.terrain.is_standable(location) then
               return stonehearth.selection.FILTER_IGNORE
            end
            return true
         end)
      :done(function(selector, location, rotation)
            _radiant.call_obj(party, 'create_attack_order_command', location, rotation)
                        :always(function()
                              selector:destroy()
                           end)
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no location')
         end)
      :always(function()
            radiant.entities.destroy_entity(cursor_entity)
         end)
      :go()   
end

function PartyEditorService:_update_parties()
   local parties = self._unit_control:get_data().parties
   for id, party in pairs(parties) do
      if not self._party_renderers[id] then
         self._party_renderers[id] = PartyRenderer(party)
      end
   end
   for id, party_renderer in pairs(self._party_renderers) do
      if not parties[id] then
         self._party_renderers[id]:destroy()
         self._party_renderers[id] = nil
      end
   end
end

function PartyEditorService:show_party_banners_command(session, response, visible)
   if visible then
      _radiant.call_obj('stonehearth.unit_control', 'get_controller_command')
         :done(function(o)
               self._unit_control = o.uri
               self._trace = self._unit_control:trace('rendering parties')
                                                   :on_changed(function()
                                                         self:_update_parties()
                                                      end)
                                                   :push_object_state()
            end)
   else
      if self._trace then
         self._trace:destroy()
         self._trace = nil
      end
      for _, party in pairs(self._party_renderers) do
         party:destroy()
      end
      self._party_renderers = {}
   end
   return true
end

return PartyEditorService
