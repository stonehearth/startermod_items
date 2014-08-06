local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local DropItemsInBackpack = class()

DropItemsInBackpack.name = 'drop items in backpack'
DropItemsInBackpack.does = 'stonehearth:drop_items_in_backpack'
DropItemsInBackpack.args = {}
DropItemsInBackpack.version = 2
DropItemsInBackpack.priority = 1

function DropItemsInBackpack:run(ai, entity, args)
   radiant.check.is_entity(entity)

   local location = radiant.entities.get_location_aligned(entity)
   local backpack_component = entity:get_component('stonehearth:backpack')
   if backpack_component then 
      while not backpack_component:is_empty() do
         local item = backpack_component:remove_first_item()
         radiant.entities.pickup_item(entity, item)
         ai:execute('stonehearth:drop_carrying_now')
      end
   end
end

return DropItemsInBackpack