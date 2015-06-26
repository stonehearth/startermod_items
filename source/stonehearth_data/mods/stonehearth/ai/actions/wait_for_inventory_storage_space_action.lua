local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local WaitForInventoryStorageSpace = class()
WaitForInventoryStorageSpace.name = 'wait for storage space'
WaitForInventoryStorageSpace.does = 'stonehearth:wait_for_inventory_storage_space'
WaitForInventoryStorageSpace.args = { }
WaitForInventoryStorageSpace.think_output = { }
WaitForInventoryStorageSpace.version = 2
WaitForInventoryStorageSpace.priority = 1

function WaitForInventoryStorageSpace:start_thinking(ai, entity, args)
   local player_id = radiant.entities.get_player_id(entity)

   self._inventory = stonehearth.inventory:get_inventory(player_id)
   if not self._inventory then
      return
   end

   self._ai = ai
   self._ready = false
   self._thinking = true
   self._listener = radiant.events.listen(self._inventory, 'stonehearth:inventory:public_storage_full_changed', self, self._on_space_changed)
   self:_on_space_changed()
end

function WaitForInventoryStorageSpace:stop_thinking()
   if self._listener then
      self._listener:destroy()
      self._listener = nil
   end
   self._inventory = nil
   self._thinking = false
end

function WaitForInventoryStorageSpace:_on_space_changed()
   local full = self._inventory:public_storage_is_full()
   if not full and self._thinking then
      self._ready = true
      self._ai:set_think_output()
   elseif full and self._ready then
      self._ai:clear_think_output()
   end
end


return WaitForInventoryStorageSpace
