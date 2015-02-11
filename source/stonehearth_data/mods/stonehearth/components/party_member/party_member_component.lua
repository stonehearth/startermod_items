local PartyMemberComponent = class()
PartyMemberComponent.__classname = 'PartyMemberComponent'

function PartyMemberComponent:initialize(entity, json)
   self._entity = entity
end

function PartyMemberComponent:destroy()
   if self._sv.party then
      self._sv.party:remove_member(self._entity:get_id())
      self._sv.party = nil
   end
end

function PartyMemberComponent:get_party()
   return self._sv.party
end

function PartyMemberComponent:set_party(party)
   self._sv.party = party
   self.__saved_variables:mark_changed()

   radiant.events.trigger_async(self._entity, 'stonehearth:party:party_changed')
end

return PartyMemberComponent
