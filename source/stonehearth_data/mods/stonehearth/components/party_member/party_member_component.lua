local PartyMemberComponent = class()
PartyMemberComponent.__classname = 'PartyMemberComponent'

function PartyMemberComponent:initialize(entity, json)   
end

function PartyMemberComponent:get_party()
   return self._sv.party
end

function PartyMemberComponent:set_party(party)
   self._sv.party = party
   self.__saved_variables:mark_changed()
end

return PartyMemberComponent
