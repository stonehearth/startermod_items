local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local DropInventoryItems = class()

DropInventoryItems.name = 'drop inventory items'
DropInventoryItems.does = 'stonehearth:drop_inventory_items'
DropInventoryItems.args = {
   
}

DropInventoryItems.version = 2
DropInventoryItems.priority = 1

function DropInventoryItems:run(ai, entity, args)
   radiant.check.is_entity(entity)

   local location = radiant.entities.get_location_aligned(entity)
   local inventory_component = entity:get_component('stonehearth:inventory')
   if inventory_component then 
      while not inventory_component:is_empty() do
         local item = inventory_component:remove_first_item()
         radiant.entities.pickup_item(entity, item)
         ai:execute('stonehearth:drop_carrying_now')
      end
   end
end

return DropInventoryItems