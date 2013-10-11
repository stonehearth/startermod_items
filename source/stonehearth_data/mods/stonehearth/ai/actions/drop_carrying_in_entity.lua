local Point3 = _radiant.csg.Point3
local DropCarryingInEntityAction = class()

DropCarryingInEntityAction.name = 'drop carrying in entity'
DropCarryingInEntityAction.does = 'stonehearth:drop_carrying_in_entity'
DropCarryingInEntityAction.priority = 5

--- Put the carried object down on another object
-- TODO: distinguish between whether it's a waist-heigh effect or not and fix crafter gather_and_craft
--@ target_location Optional, location on target entity
function DropCarryingInEntityAction:run(ai, entity, target_entity, target_location)
   radiant.check.is_entity(entity)
   local location = radiant.entities.get_world_grid_location(target_entity)

   if radiant.entities.is_carrying(entity) then
      radiant.entities.turn_to_face(entity, location)
      if not target_location or target_location.y == 0 then
         ai:execute('stonehearth:run_effect', 'carry_putdown')
      else
         --if it's not 0, it must be higher
         ai:execute('stonehearth:run_effect', 'carry_putdown_on_table')
      end
      radiant.entities.put_carrying_in_entity(entity, target_entity, target_location)
   end
end

return DropCarryingInEntityAction