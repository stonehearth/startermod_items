local entity_forms_lib = require 'lib.entity_forms.entity_forms_lib'

local Entity = _radiant.om.Entity
local PickupItemFromBackpack = class()

PickupItemFromBackpack.name = 'pickup item from backpack'
PickupItemFromBackpack.does = 'stonehearth:pickup_item'
PickupItemFromBackpack.args = {
   item = Entity,
}
PickupItemFromBackpack.version = 2
PickupItemFromBackpack.priority = 1

function PickupItemFromBackpack:start_thinking(ai, entity, args)
   ai:monitor_carrying()
   if ai.CURRENT.carrying then
      return
   end

   local backpack = entity:get_component('stonehearth:storage')
   if not backpack then
      return
   end

   -- if we're instructed to pick up the root form of an item,
   -- but the item is currently in iconic form, just go right for
   -- the icon.
   self._item = args.item
   local root, iconic, _ = entity_forms_lib.get_forms(self._item)
   if root and iconic then
      self._item = iconic
   end

   if not backpack:contains_item(self._item:get_id()) then
      return
   end
   
   ai.CURRENT.carrying = self._item
   ai:set_think_output()
end

function PickupItemFromBackpack:run(ai, entity, args)
   -- if we're already carrying the item, there's no need to pull it out
   -- of our pack.  if we're not but carrying something else, make sure
   -- we get rid of it first.  - tony
   if stonehearth.ai:prepare_to_pickup_item(ai, entity, self._item) then
      return
   end
   assert(not radiant.entities.get_carrying(entity))

   local success = entity:get_component('stonehearth:storage')
                           :remove_item(self._item:get_id())
   if not success then
      ai:abort('item not found in backpack')
   end  
   stonehearth.ai:pickup_item(ai, entity, self._item)
end

return PickupItemFromBackpack
