local DropCarryingAction = class()

DropCarryingAction.name = 'stonehearth.actions.drop_carrying'
DropCarryingAction.does = 'stonehearth.activities.drop_carrying'
DropCarryingAction.priority = 5
--TODO we need a scale to  describe relative importance

--[[
   Put the object we're carrying down at a location
   location: the coordinates at which to drop off the object
]]
function DropCarryingAction:run(ai, entity, location)
   radiant.check.is_entity(entity)
   radiant.check.is_a(location, RadiantIPoint3)

   if radiant.entities.is_carrying(entity) then
      --TODO: add this back when
      radiant.entities.turn_to_face(entity, location)
      ai:execute('stonehearth.activities.run_effect', 'carry_putdown')
      --[[
      --TODO: replace with ai:execute('stonehearth.activities.run_effect,''carry_putdown')
      --It does not seem to be working right now, but this does:
      self._effect = radiant.effects.run_effect(entity, 'carry_putdown')
      ai:wait_until(function()
         return self._effect:finished()
      end)
      self._effect = nil
      --]]
      radiant.entities.drop_carrying(entity, location)
   end
end

return DropCarryingAction