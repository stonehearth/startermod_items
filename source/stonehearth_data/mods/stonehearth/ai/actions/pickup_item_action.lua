local entity_forms_lib = require 'lib.entity_forms.entity_forms_lib'
local Entity = _radiant.om.Entity
local PickupItem = class()

PickupItem.name = 'pickup an item'
PickupItem.does = 'stonehearth:pickup_item'
PickupItem.args = {
   item = Entity,
}
PickupItem.version = 2
PickupItem.priority = 1

function PickupItem:start_thinking(ai, entity, args)
   local item = args.item

   if ai.CURRENT.carrying then
      -- cannot pick something up if we're already carrying!
      return
   end

   -- if we're instructed to pick up the root form of an item,
   -- but the item is currently in iconic form, just go right for
   -- the icon.  picking it up would have convereted it to an icon anyway
   local root, iconic, _ = entity_forms_lib.get_forms(args.item)
   if root and iconic then
      local root_parent = root:add_component('mob'):get_parent()
      if not root_parent then
         local iconic_parent = iconic:add_component('mob'):get_parent()
         if iconic_parent then
            item = iconic
         end
      end
   end

   ai:set_think_output({
         item = item
      })
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItem)
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.item })
         :execute('stonehearth:goto_entity', { entity = ai.BACK(2).item })
         :execute('stonehearth:pickup_item_adjacent', { item = ai.BACK(3).item })
