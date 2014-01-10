local Point3 = _radiant.csg.Point3
local DropCarryingAdjacent = class()

DropCarryingAdjacent.name = 'drop carrying adjacent'
DropCarryingAdjacent.does = 'stonehearth:drop_carrying_adjacent'
DropCarryingAdjacent.args = {
   location = Point3      -- where to drop it
}
DropCarryingAdjacent.version = 2
DropCarryingAdjacent.priority = 1

--[[
   Put the object we're carrying down at a location
   location: the coordinates at which to drop off the object
]]
function DropCarryingAdjacent:start_thinking(ai, entity, args)
   -- todo: ASSERT we're adjacent!
   ai:set_think_output()
end

function DropCarryingAdjacent:run(ai, entity, args)
   local location = args.location

   radiant.check.is_entity(entity)
   radiant.check.is_a(location, Point3)

   if not radiant.entities.get_carrying(entity) then
      ai:abort('cannot drop item not carrying one!')
   end
   if not radiant.entities.is_adjacent_to(entity, location) then
      ai:abort('%s is not adjacent to %s', tostring(entity), tostring(location))
   end
   
   radiant.entities.turn_to_face(entity, location)
   ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })
   radiant.entities.drop_carrying_on_ground(entity, location)
end

return DropCarryingAdjacent
