local Entity = _radiant.om.Entity
local PutCarryingInBackpack = class()

PutCarryingInBackpack.name = 'put carrying in backpack'
PutCarryingInBackpack.does = 'stonehearth:put_carrying_in_backpack'
PutCarryingInBackpack.args = {}
PutCarryingInBackpack.version = 2
PutCarryingInBackpack.priority = 1

function PutCarryingInBackpack:start_thinking(ai, entity, args)
   local storage_component = entity:add_component('stonehearth:storage')
   if ai.CURRENT.carrying == nil or not storage_component:is_full() then
      ai.CURRENT.carrying = nil
      ai:set_think_output()
   end
end

function PutCarryingInBackpack:run(ai, entity, args)
   radiant.check.is_entity(entity)

   if not radiant.entities.get_carrying(entity) then
      return
   end

   local storage_component = entity:add_component('stonehearth:storage')
   if storage_component:is_full() then
      ai:abort('cannot put carrying in inventory when inventory is full')
      return
   end

   local item = radiant.entities.remove_carrying(entity)
   radiant.check.is_entity(item)

   storage_component:add_item(item)
end

return PutCarryingInBackpack
