local Entity = _radiant.om.Entity

local FindEquipmentUpgrade = class()
FindEquipmentUpgrade.name = 'find equipment upgrade'
FindEquipmentUpgrade.does = 'stonehearth:find_equipment_upgrade'
FindEquipmentUpgrade.args = {}
FindEquipmentUpgrade.think_output = {
   item = Entity,       -- the upgrade
}
FindEquipmentUpgrade.version = 2
FindEquipmentUpgrade.priority = 1

function FindEquipmentUpgrade:start_thinking(ai, entity, args)
   self._ai = ai
   self._ready = false
   self._inventory = stonehearth.inventory:get_inventory(entity)

   if self._inventory then
      self._equipment = entity:add_component('stonehearth:equipment')

      local tracker = self._inventory:add_item_tracker('stonehearth:equipment_tracker')
      for id, piece in pairs(tracker:get_tracking_data()) do
         self:_check_equipment_piece(piece)
         if self._ready then
            return
         end
      end   
      radiant.events.listen(self._inventory, 'stonehearth:item_added', self, self._on_inventory_item_added)
   end
end

function FindEquipmentUpgrade:_on_inventory_item_added(e)
   self:_check_equipment_piece(e.item)
end

function FindEquipmentUpgrade:_check_equipment_piece(item)
   if not self._ready then
      local pip = item:get_component('stonehearth:placeable_item_proxy')
      if pip then
         local equipment = pip:get_full_sized_entity()
                                 :get_component('stonehearth:equipment_piece')
         if equipment then
            local slot = equipment:get_slot()
            if slot then
               local equipped = self._equipment:get_item_in_slot(slot)
               if not equipped or equipped:get_component('stonehearth:equipment_piece'):get_ilevel() < equipment:get_ilevel() then
                  self._ready = true
                  self._ai:set_think_output({
                        item = item,
                     })
                  return
               end
            end
         end
      end
   end
end

function FindEquipmentUpgrade:stop_thinking(ai, entity)
   self._ai = nil
   self._ready = false
   if self._inventory then
      radiant.events.unlisten(self._inventory, 'stonehearth:item_added', self, self._on_inventory_item_added)
   end
end

return FindEquipmentUpgrade
