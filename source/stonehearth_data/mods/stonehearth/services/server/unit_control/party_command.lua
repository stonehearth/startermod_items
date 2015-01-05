local PartyCommand = class()

local VALID_COMBAT_STANCES = {
   default     = true,
   defensive   = true,
   aggressive  = true,
}

local VALID_ACTIONS = {
   guard       = true,
   patrol      = true,
   steal       = true,
   destroy     = true,   
}

function PartyCommand:initialize(party, id, action, target)
   self._sv.id = id
   self._sv.party = party
   self._sv.action = action
   self._sv.target = target
   self._sv.travel_stance = 'default'
   self._sv.arrived_stance = 'default'
end

function PartyCommand:set_target(target)
   self._sv.target = target
   self.__saved_variables:mark_changed()
   return self
end

function PartyCommand:get_target(target)
   return self._sv.target
end

function PartyCommand:set_travel_stance(cs)
   assert(VALID_COMBAT_STANCES[cs])
   self._sv.travel_stance = cs
   return self
end

function PartyCommand:get_travel_stance(target)
   return self._sv.travel_stance
end

function PartyCommand:set_arrived_stance(cs)
   assert(VALID_COMBAT_STANCES[cs])
   self._sv.arrived_stance = cs
   return self
end

function PartyCommand:set_action(action)
   assert(VALID_ACTIONS[action])
   self._sv.action = action
   return self   
end

function PartyCommand:go()
   self._sv.party:_start_command(self)
   return self
end

function PartyCommand:destroy()
   radiant.not_yet_implemented()
end

function PartyCommand:_set_state(state)
   self._sv.state = state
   self.__saved_variables:mark_changed()
end

return PartyCommand
