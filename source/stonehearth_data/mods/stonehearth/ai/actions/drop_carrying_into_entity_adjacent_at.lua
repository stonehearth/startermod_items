local Entity = _radiant.om.Entity
local DropCarryingIntoEntityAdjacent = class()
local Point3 = _radiant.csg.Point3

DropCarryingIntoEntityAdjacent.name = 'drop carrying into entity adjacent at'
DropCarryingIntoEntityAdjacent.does = 'stonehearth:drop_carrying_into_entity_adjacent'
DropCarryingIntoEntityAdjacent.args = {
   entity = Entity      -- what to drop it into
}
DropCarryingIntoEntityAdjacent.version = 2
DropCarryingIntoEntityAdjacent.priority = 2

--[[
   Like drop_carrying_into_entity_adjacent, except that we pick what location in 
   the target to put the object we're dropping. And we run the a specified drop animation 
   instead of the normal drop animation. 
]]
function DropCarryingIntoEntityAdjacent:start_thinking(ai, entity, args)
   -- todo: ASSERT we're adjacent!
   -- if the target is not a table, then just return
   local container = args.entity
   local container_entity_data = radiant.entities.get_entity_data(container, 'stonehearth:table')
   if not container_entity_data then
      return
   end
   local offset = container_entity_data['drop_offset']
   if offset then
      self._drop_offset = Point3(offset.x, offset.y, offset.z)
   end
   self._drop_effect = container_entity_data['drop_effect']
   if not self._drop_effect then
      self._drop_effect = 'carry_putdown_on_table'
   end

   ai:set_think_output()
end

function DropCarryingIntoEntityAdjacent:run(ai, entity, args)
   local container = args.entity
   local carrying = radiant.entities.get_carrying(entity)

   if not carrying then
      ai:abort('cannot drop item not carrying one!')
   end
   if not radiant.entities.is_adjacent_to(entity, container) then
      ai:abort('%s is not adjacent to %s', tostring(entity), tostring(container))
   end
   
   radiant.entities.turn_to_face(entity, container)
   ai:execute('stonehearth:run_effect', { effect =  self._drop_effect})  
   radiant.entities.put_carrying_into_entity(entity, container)
   carrying:add_component('mob'):set_location_grid_aligned(self._drop_offset)
end

return DropCarryingIntoEntityAdjacent
