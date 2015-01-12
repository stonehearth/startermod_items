local HoldFormationAction = class()

HoldFormationAction.name = 'hold party formation'
HoldFormationAction.does = 'stonehearth:party:hold_formation'
HoldFormationAction.args = {
   party = 'table',     -- technically a party controller..
}
HoldFormationAction.version = 2
HoldFormationAction.priority = stonehearth.constants.priorities.party.HOLD_FORMATION

function HoldFormationAction:start_thinking(ai, entity, args)   
   local location = args.party:get_formation_location_for(entity)
   if location then
      ai:set_think_output({ location = location })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(HoldFormationAction)
         :execute('stonehearth:party:hold_formation_location', { location = ai.PREV.location  })
