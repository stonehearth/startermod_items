local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local PutCarryingInInventory = class()

PutCarryingInInventory.name = 'put carrying in inventory'
PutCarryingInInventory.does = 'stonehearth:put_carrying_in_inventory'
PutCarryingInInventory.args = {
   
}
PutCarryingInInventory.think_output = {
   item = Entity,          -- what got dropped
}
PutCarryingInInventory.version = 2
PutCarryingInInventory.priority = 1

function PutCarryingInInventory:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying ~= nil then
      -- todo: ASSERT we're adjacent!
      local carrying = ai.CURRENT.carrying
      ai.CURRENT.carrying = nil
      ai:set_think_output({ item = carrying })

      --todo, check that there's bag space
   end
end

function PutCarryingInInventory:run(ai, entity, args)
   radiant.check.is_entity(entity)

   if not radiant.entities.get_carrying(entity) then
      ai:abort('cannot put carrying in inventory if youre not carrying anything!')
      return
   end

   local item = radiant.entities.remove_carrying(entity)
   radiant.check.is_entity(item)
   
   local inventory_component = entity:add_component('stonehearth:inventory')
   inventory_component:add_item(item)

end


return PutCarryingInInventory
