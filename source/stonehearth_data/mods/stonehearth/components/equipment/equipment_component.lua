local EquipmentComponent = class()

function EquipmentComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.equipped_items then
      self._sv.equipped_items = {}
   end
   self._kill_listener = radiant.events.listen(entity, 'stonehearth:kill_event', self, self._on_kill_event)
end

function EquipmentComponent:destroy()
   -- xxx, revoke injected ais for all equipped items
   --stonehearth.ai:revoke_injected_ai(self._injected_ai_token)
   self._kill_listener:destroy()
   self._kill_listener = nil
end

--When we've been killed, dump things that are >0 iLevel on the ground
function EquipmentComponent:_on_kill_event()
   self:drop_equipment()
end

function EquipmentComponent:drop_equipment()
   if self._sv.equipped_items then
      for key, item in pairs(self._sv.equipped_items) do
         self:unequip_item(item)
         local ep = item:get_component('stonehearth:equipment_piece')
         if ep and ep:get_ilevel() > 0 then 
            local location = radiant.entities.get_world_grid_location(self._entity)
            local placement_point = radiant.terrain.find_placement_point(location, 1, 4)
            radiant.terrain.place_entity(item, placement_point)
         end
      end
   end
end

-- use pairs (not ipairs) to iterate through all the itmes
function EquipmentComponent:get_all_items()
   return self._sv.equipped_items
end

function EquipmentComponent:get_item_in_slot(slot)
   return self._sv.equipped_items[slot]
end

function EquipmentComponent:equip_item(item)
   local ep

   -- if someone tries to equip a proxy, equip the full-sized item instead
   assert(radiant.check.is_entity(item))
   local proxy = item:get_component('stonehearth:iconic_form')
   if proxy then
      item = proxy:get_root_entity()
   end
   local ep = item:get_component('stonehearth:equipment_piece')
   assert(ep, 'item is not an equipment piece')

   
   local slot = ep:get_slot()
   
   if not slot then
      -- no slot specified, make up a magic slot name
      slot = 'no_slot_' .. #self._sv.equipped_items
   end

   -- unequip previous item in slot first, then assign the item to the slot
   local unequipped_item = self._sv.equipped_items[slot]
   if unequipped_item then
      self:unequip_item(unequipped_item)
   end
   self._sv.equipped_items[slot] = item
   

   ep:equip(self._entity)
   self.__saved_variables:mark_changed()
   self:_trigger_equipment_changed()

   return unequipped_item
end

-- takes an enity or a uri
function EquipmentComponent:unequip_item(equipped_item)
   local uri

   if type(equipped_item) == 'string' then
      uri = equipped_item
   else
      uri = equipped_item:get_uri()
   end

   for key, item in pairs(self._sv.equipped_items) do
      local item_uri = item:get_uri()

      if item_uri == uri then
         -- remove the item from the slot
         self._sv.equipped_items[key] = nil

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
