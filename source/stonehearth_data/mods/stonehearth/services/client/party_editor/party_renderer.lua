local PartyRenderer = class()

function PartyRenderer:__init(party)
   self._party = party
   self._attack_banner = {
      uri = 'stonehearth:attack_order_banner'
   }
   self._defend_banner = {
      uri = 'stonehearth:attack_order_banner'
   }

   self._trace = party:trace('rendering party')
                     :on_changed(function()
                           self:_update()
                        end)
                     :push_object_state()
end

function PartyRenderer:destroy()
   self:_destroy_banner(self._attack_banner)
   self:_destroy_banner(self._defend_banner)

   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function PartyRenderer:_update()
   local party = self._party:get_data()

   self:_update_banner(party.party_location, self._attack_banner)
end

function PartyRenderer:_update_banner(location, banner)
   if location then
      self:_create_banner(banner)
      radiant.entities.move_to(banner.entity, location)
   else
      self:_destroy_banner(banner)
   end
end

function PartyRenderer:_create_banner(banner)
   if not banner.entity then
      banner.entity = radiant.entities.create_entity(banner.uri)
      banner.entity:add_component('render_info')
                        :set_material('materials/ghost_item.xml')

      banner.render_entity = _radiant.client.create_render_entity(1, banner.entity)
   end
end

function PartyRenderer:_destroy_banner(banner)
   if banner.entity then
      radiant.entities.destroy_entity(banner.entity)
      banner.entity = nil
      banner.render_entity = nil
   end
end

return PartyRenderer

