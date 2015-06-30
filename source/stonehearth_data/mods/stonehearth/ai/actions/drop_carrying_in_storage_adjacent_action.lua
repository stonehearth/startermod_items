local Entity = _radiant.om.Entity
local DropCarryingInStorageAdjacent = class()

DropCarryingInStorageAdjacent.name = 'drop carrying in storage'
DropCarryingInStorageAdjacent.does = 'stonehearth:drop_carrying_in_storage_adjacent'
DropCarryingInStorageAdjacent.args = {
   storage = Entity,
}
DropCarryingInStorageAdjacent.think_output = {
   item = Entity,
}
DropCarryingInStorageAdjacent.version = 2
DropCarryingInStorageAdjacent.priority = 1

function DropCarryingInStorageAdjacent:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying ~= nil then
      local sc = args.storage:add_component('stonehearth:storage')
      if not sc:is_full() then
         local carrying = ai.CURRENT.carrying
         ai.CURRENT.carrying = nil
         ai:set_think_output({ item = carrying })
      end
   end
end

function DropCarryingInStorageAdjacent:run(ai, entity, args)
   radiant.check.is_entity(entity)

   if not radiant.entities.get_carrying(entity) then
      ai:abort('cannot put carrying in inventory if you are not carrying anything')
      return
   end

   local sc = args.storage:add_component('stonehearth:storage')
   if sc:is_full() then
      ai:abort('cannot put carrying in inventory when inventory is full')
      return
   end

   if not radiant.entities.is_adjacent_to(entity, args.storage) then
      ai:abort('%s is not adjacent to %s', tostring(entity), tostring(args.storage))
   end

   radiant.entities.turn_to_face(entity, args.storage)
   local storage_location = radiant.entities.get_world_grid_location(args.storage)
   ai:execute('stonehearth:run_putdown_effect', { location = storage_location })

   local item = radiant.entities.remove_carrying(entity)
   sc:add_item(item)
end

return DropCarryingInStorageAdjacent
