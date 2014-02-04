local Entity = _radiant.om.Entity
local DropCarryingIntoEntityAdjacent = class()

DropCarryingIntoEntityAdjacent.name = 'drop carrying into entity adjacent'
DropCarryingIntoEntityAdjacent.does = 'stonehearth:drop_carrying_into_entity_adjacent'
DropCarryingIntoEntityAdjacent.args = {
   entity = Entity      -- what to drop it into
}
DropCarryingIntoEntityAdjacent.version = 2
DropCarryingIntoEntityAdjacent.priority = 1

--[[
   Put the object we're carrying down at a location
   location: the coordinates at which to drop off the object
]]
function DropCarryingIntoEntityAdjacent:start_thinking(ai, entity, args)
   -- todo: ASSERT we're adjacent!
   ai:set_think_output()
end

function DropCarryingIntoEntityAdjacent:run(ai, entity, args)
   local container = args.entity

   if not radiant.entities.get_carrying(entity) then
      ai:abort('cannot drop item not carrying one!')
   end
   if not radiant.entities.is_adjacent_to(entity, container) then
      ai:abort('%s is not adjacent to %s', tostring(entity), tostring(container))
   end
   
   radiant.entities.turn_to_face(entity, container)
   ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })
   radiant.entities.put_carrying_into_entity(entity, container)
end

return DropCarryingIntoEntityAdjacent
