local Point3 = _radiant.csg.Point3
local DropCarryingAction = class()

DropCarryingAction.name = 'drop carrying'
DropCarryingAction.does = 'stonehearth:drop_carrying'
DropCarryingAction.args = {
   Point3      -- where to drop it
}
DropCarryingAction.run_args = {
   Point3      -- where to drop it
}
DropCarryingAction.version = 2
DropCarryingAction.priority = 1

--[[
   Put the object we're carrying down at a location
   location: the coordinates at which to drop off the object
]]
function DropCarryingAction:start_background_processing(ai, entity, location)
   -- todo: ASSERT we're adjacent!
   ai:complete_background_processing(location)
end

function DropCarryingAction:run(ai, entity, location)
   radiant.check.is_entity(entity)
   radiant.check.is_a(location, Point3)

   if radiant.entities.is_carrying(entity) then
      radiant.entities.turn_to_face(entity, location)
      ai:execute('stonehearth:run_effect', 'carry_putdown')
      radiant.entities.drop_carrying_on_ground(entity, location)
   end
end

return DropCarryingAction
