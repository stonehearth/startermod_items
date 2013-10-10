local Point3 = _radiant.csg.Point3
local DropCarryingOnEntityAction = class()

DropCarryingOnEntityAction.name = 'stonehearth.drop_carrying_on_entity'
DropCarryingOnEntityAction.does = 'stonehearth.drop_carrying_on_entity'
DropCarryingOnEntityAction.priority = 5

--- Put the carried object down on another object
-- TODO: distinguish between whether it's a waist-heigh effect or not and fix crafter gather_and_craft
--@ target_location Optional, location on target entity
function DropCarryingOnEntityAction:run(ai, entity, target_entity, target_location)
   radiant.check.is_entity(entity)
   local location = radiant.entities.get_world_grid_location(target_entity)

   if radiant.entities.is_carrying(entity) then
      radiant.entities.turn_to_face(entity, location)
      ai:execute('stonehearth.run_effect', 'carry_putdown')
      radiant.entities.drop_carrying_on_object(entity, target_entity, target_location)
   end

   entity:add_component('stonehearth:posture'):unset_posture('carrying')
end

return DropCarryingOnEntityAction