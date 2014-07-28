local EquipmentComponent = class()

function EquipmentComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.equipped_items then
      self._sv.equipped_items = {}
   end
end

function EquipmentComponent:destroy()
   -- xxx, revoke injected ais for all equipped items
   --stonehearth.ai:revoke_injected_ai(self._injected_ai_token)
end

-- use pairs (not ipairs) to iterate through all the itmes
function EquipmentComponent:get_all_items()
   return self._sv.equipped_items
end

function EquipmentComponent:get_item_in_slot(slot)
   return self._sv.equipped_items[slot]
end

-- expect an entity, but if you pass a uri we'll make a new one
-- if slot is specified, unequips existing item in slot first and returns the unequipped item
function EquipmentComponent:equip_item(item)
   local ep

   -- if someone tries to equip a proxy, equip the full-sized item instead
   assert(radiant.check.is_entity(item))
   local proxy = item:get_component('stonehearth:placeable_item_proxy')
   if proxy then
      item = proxy:get_full_sized_entity()
   end
   local ep = item:get_component('stonehearth:equipment_piece')
   assert(ep, 'item is not an equipment piece')

   local unequipped_item = nil
   local slot = ep:get_slot()
   if not slot then
      -- no slot, just add to the array
      table.insert(self._sv.equipped_items, item)
   else
      -- unequip previous item in slot first, then assign the item to the slot
      unequipped_item = self._sv.equipped_items[slot]
      if unequipped_item then
         self:unequip_item(unequipped_item)
      end
      self._sv.equipped_items[slot] = item
   end   

   ep:equip(self._entity)
   self.__saved_variables:mark_changed()
   self:_trigger_equipment_changed()

   return unequipped_item
end

-- expect a uri or an enitity
function EquipmentComponent:unequip_item(equipped_item)
   local uri = equipped_item:get_uri()

   for key, item in pairs(self._sv.equipped_items) do
      local item_uri = item:get_uri()

      if item_uri == uri then
         if type(key) == 'number' then
            -- remove from the array
            table.remove(self._sv.equipped_items, key)
         else
            -- remove the item from the slot
            self._sv.equipped_items[key] = nil
         end

         item:get_component('stonehearth:equipment_piece'):unequip();
         self.__saved_variables:mark_changed()
         self:_trigger_equipment_changed()

         return item
      end
   end
end

function EquipmentComponent:_trigger_equipment_changed()
   radiant.events.trigger_async(self._entity, 'stonehearth:equipment_changed')
end

return EquipmentComponent
