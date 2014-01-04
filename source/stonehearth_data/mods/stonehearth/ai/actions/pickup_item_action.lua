local PickupItem = class()

PickupItem.name = 'pickup an item'
PickupItem.does = 'stonehearth:pickup_item'
PickupItem.version = 1
PickupItem.priority = 5
--TODO we need a scale to  describe relative importance

--[[
   ai: the AI for all entities
   entity: the person doing the picking up
   item: the thing to pick up
]]
function PickupItem:run(ai, entity, item)
   radiant.check.is_entity(item)
   local carry_block = entity:get_component('carry_block')

   local carrying = carry_block:get_carrying()
   if carrying then
      if carrying:get_id() ~= item:get_id() then
         ai:abort('cannot pick up another item while carrying one!')
      end
      return -- already carrying it!  nothing to do...
   end
   
   -- figure out what kind of an item we're talking about...
   local mob = item:get_component('mob')
   local parent = mob:get_parent()
   if not parent then
      ai:abort('item has no parent... no way to find path to it!')
   end
   
   if parent:get_id() == radiant.entities.get_root_entity():get_id() then
      -- on the ground.  awesome.
      ai:execute('stonehearth:pickup_item_on_ground', item)
   else
      -- xxx: assume it's on a table for now!
      ai:execute('stonehearth:pickup_item_on_table', item, parent)
   end
end

return PickupItem
