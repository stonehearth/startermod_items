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
   self._entity = entity
   self._ready = false
   self._inventory = stonehearth.inventory:get_inventory(entity)

   if self._inventory then
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
      --- if we can't acquire a lease to the item, just bail right away.
      if not stonehearth.ai:can_acquire_ai_lease(item, self._entity) then
         return
      end

      -- if the item is not an equipment piece, bail.  equipment pieces are always
      -- placed on the ground via a proxy and have an equipment_piece component
      local pip = item:get_component('stonehearth:iconic_form')
      if not pip then
         return
      end

      local equipment = pip:get_root_entity()
                              :get_component('stonehearth:equipment_piece')
      if not equipment then
         return
      end

      -- if this equipment piece is an upgrade for the current unit, go ahead and
      -- unblock the ai
      if equipment:is_upgrade_for(self._entity) then
         self._ready = true
         self._ai:set_think_output({
               item = item,
            })
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
