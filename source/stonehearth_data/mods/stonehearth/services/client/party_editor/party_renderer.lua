local PartyRenderer = class()

function PartyRenderer:__init(party)
   self._party = party
   self._banners = {
      attack = {},
      defend = {}
   }
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
   self:_destroy_banner('attack')
   self:_destroy_banner('defend')

   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function PartyRenderer:set_selected(enabled)
   self._selected = enabled
   self:_update_material('attack')
   self:_update_material('defend')   
end

function PartyRenderer:_update()
   local party = self._party:get_data()

   self:_update_banner('attack', party)
   self:_update_banner('defend', party)
end

function PartyRenderer:_update_banner(banner_type, party)
   local banner = self._banners[banner_type]
   local location = party.banners[banner_type]
   if location then
      local entity = self:_create_banner(banner, banner_type)
      radiant.entities.move_to(entity, location)
   else
      self:_destroy_banner(banner_type)
   end
end

function PartyRenderer:_create_banner(banner, banner_type)
   if not banner.entity then
      local uri = string.format('stonehearth:%s_order_banner', banner_type)
      banner.entity = radiant.entities.create_entity(uri)
      banner.render_entity = _radiant.client.create_render_entity(1, banner.entity)
      self:_update_material(banner_type)
   end
   return banner.entity
end

function PartyRenderer:_destroy_banner(banner_type)
   local banner = self._banners[banner_type]
   if banner.entity then
      radiant.entities.destroy_entity(banner.entity)
      banner.entity = nil
      banner.render_entity = nil
   end
end

function PartyRenderer:_update_material(banner_type)
   local banner = self._banners[banner_type]
   if banner.entity then
      local material = self._selected and 'materials/banner_selected.material.json' or 'materials/ghost_item.json'
      banner.entity:add_component('render_info')
                        :set_material(material)
   end
end


return PartyRenderer

