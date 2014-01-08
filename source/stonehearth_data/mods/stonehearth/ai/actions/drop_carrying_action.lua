local Point3 = _radiant.csg.Point3
local DropCarryingAction = class()

DropCarryingAction.name = 'drop carrying'
DropCarryingAction.does = 'stonehearth:drop_carrying'
DropCarryingAction.args = {
   location = Point3      -- where to drop it
}
DropCarryingAction.run_args = {
   location = Point3      -- where to drop it
}
DropCarryingAction.version = 2
DropCarryingAction.priority = 1

--[[
   Put the object we're carrying down at a location
   location: the coordinates at which to drop off the object
]]
function DropCarryingAction:start_thinking(ai, entity, args)
   -- todo: ASSERT we're adjacent!
   ai:set_run_arguments(args)
end

function DropCarryingAction:run(ai, entity, args)
   local location = args.location

   radiant.check.is_entity(entity)
   radiant.check.is_a(location, Point3)

   if radiant.entities.is_carrying(entity) then
      radiant.entities.turn_to_face(entity, location)
      ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })
      radiant.entities.drop_carrying_on_ground(entity, location)
   end
end

return DropCarryingAction
