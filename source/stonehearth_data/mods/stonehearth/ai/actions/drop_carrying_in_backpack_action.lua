local Entity = _radiant.om.Entity
local DropCarryingInBackpack = class()

DropCarryingInBackpack.name = 'drop carrying in backpack'
DropCarryingInBackpack.does = 'stonehearth:drop_carrying_in_backpack'
DropCarryingInBackpack.args = {
   backpack_entity = Entity,
}
DropCarryingInBackpack.think_output = {
   item = Entity,
}
DropCarryingInBackpack.version = 2
DropCarryingInBackpack.priority = 1

function DropCarryingInBackpack:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying ~= nil then
      local backpack_component = args.backpack_entity:add_component('stonehearth:backpack')
      if not backpack_component:is_full() then
         local carrying = ai.CURRENT.carrying
         ai.CURRENT.carrying = nil
         ai:set_think_output({ item = carrying })
      end
   end
end

function DropCarryingInBackpack:run(ai, entity, args)
   radiant.check.is_entity(entity)

   if not radiant.entities.get_carrying(entity) then
      ai:abort('cannot put carrying in inventory if you are not carrying anything')
      return
   end

   local backpack_component = args.backpack_entity:add_component('stonehearth:backpack')
   if backpack_component:is_full() then
      ai:abort('cannot put carrying in inventory when inventory is full')
      return
   end

   radiant.entities.turn_to_face(entity, args.backpack_entity)
   ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })

   local item = radiant.entities.remove_carrying(entity)
   radiant.check.is_entity(item)
   
   backpack_component:add_item(item)
end

return DropCarryingInBackpack
