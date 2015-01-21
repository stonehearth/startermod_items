local UnitController = class()

function UnitController:initialize(player_id)
   self._sv.parties = {}
   self._sv.commands = {}
   self._sv.next_id = 1000
   self._sv.player_id = player_id
   self._sv.next_party_ordinal = 1
end

function UnitController:create_party()
   local party_id = self:_get_next_id()
   local party_ord = self:_get_next_ordinal()

   local party = radiant.create_controller('stonehearth:unit_control:party', self, party_id, party_ord)
   self._sv.parties[party_id] = party
   self.__saved_variables:mark_changed()

   return party
end

function UnitController:disband_party(id)
   local party = self._sv.parties[id]
   if party then
      radiant.destroy_controller(party)
      self._sv.parties[id] = nil
      self.__saved_variables:mark_changed()
   end
end

function UnitController:get_player_id()
   return self._sv.player_id
end

function UnitController:_get_next_id()
   local id = self._sv.next_id
   self._sv.next_id = id + 1
   return id
end

function UnitController:_get_next_ordinal()
   local ord = self._sv.next_party_ordinal
   self._sv.next_party_ordinal = ord + 1
   return ord
end

return UnitController

