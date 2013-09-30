local Point3 = _radiant.csg.Point3
local DropCarryingAction = class()

DropCarryingAction.name = 'stonehearth.actions.drop_carrying'
DropCarryingAction.does = 'stonehearth.drop_carrying'
DropCarryingAction.priority = 5
--TODO we need a scale to  describe relative importance

--[[
   Put the object we're carrying down at a location
   location: the coordinates at which to drop off the object
]]
function DropCarryingAction:run(ai, entity, location)
   radiant.check.is_entity(entity)
   radiant.check.is_a(location, Point3)

   if radiant.entities.is_carrying(entity) then
      radiant.entities.turn_to_face(entity, location)
      ai:execute('stonehearth.run_effect', 'carry_putdown')
      radiant.entities.drop_carrying(entity, location)
   end
end

return DropCarryingAction