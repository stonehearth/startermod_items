local EquipmentComponent = class()

function EquipmentComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.equipped_items then
      self._sv.equipped_items = {
         n = 0
      }
   end
end

-- expect an entity, but if you pass a uri we'll make a new one
function EquipmentComponent:equip_item(item)
   if type(item) == 'string' then
      item = radiant.entities.create_entity(item)
   end
   assert(radiant.check.is_entity(item))

   local ep = item:get_component('stonehearth:equipment_piece');
   if ep then
      ep:equip(self._entity)
      table.insert(self._sv.equipped_items, item)
   end
end

-- expect a uri or an enitity
function EquipmentComponent:unequip_item(uri)
   if type(uri) ~= 'string' then
      uri = uri:get_uri()
   end

   for i, item in pairs(self._sv.equipped_items) do
      local item_uri = item:get_uri()
      if item_uri == uri then
         item:get_component('stonehearth:equipment_piece'):unequip();
         table.remove(self._sv.equipped_items, i)
         self.__saved_variables:mark_changed()
         return item
      end
   end
end

function EquipmentComponent:destroy()
   -- xxx, revoke injected ais for all equipped items
   --stonehearth.ai:revoke_injected_ai(self._injected_ai_token)
end


return EquipmentComponent