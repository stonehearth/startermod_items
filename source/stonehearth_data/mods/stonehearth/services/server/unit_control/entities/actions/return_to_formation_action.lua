local Point3 = _radiant.csg.Point3

local ReturnToFormationAction = class()

ReturnToFormationAction.name = 'return to formation'
ReturnToFormationAction.does = 'stonehearth:party:hold_formation_location'
ReturnToFormationAction.args = {
   location = Point3    -- world location of formation position
}
ReturnToFormationAction.version = 2
ReturnToFormationAction.priority = stonehearth.constants.priorities.party.hold_formation.RETURN_TO_FORMATION

function ReturnToFormationAction:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self._location = args.location

   -- we are almost certainly too far away.  check once before creating the trace.
   if self:_check_position() then
      return
   end
   
   -- no?  ok, set_think_output whenever we get there   
   self._trace = radiant.entities.trace_grid_location(entity, 'hold formation')
                     :on_changed(function()
                           self:_check_position()
                        end)
end


function ReturnToFormationAction:stop_thinking()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function ReturnToFormationAction:_check_position()
   local location = radiant.entities.get_world_grid_location(self._entity)
   if location:distance_to(self._location) <= 2 then
      return false
   end
   
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
   self._ai:set_think_output({ location = self._location })
   return true
end

local ai = stonehearth.ai
return ai:create_compound_action(ReturnToFormationAction)
         :execute('stonehearth:goto_location', { reason = 'return to formation', location = ai.PREV.location  })
