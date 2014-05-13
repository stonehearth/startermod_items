local Entity = _radiant.om.Entity

local Town = require 'services.server.town.town'

local TownService = class()

function TownService:initialize()
   self._sv = self.__saved_variables:get_data() 
   if not self._sv.towns then
      self._sv.towns = {}
   else
      for player_id, ss in pairs(self._sv.towns) do
         self._sv.towns[player_id] = Town(nil, ss)
      end
   end
   radiant.log.write('town', 0, 'initialize not implemented for town service!')
end

function TownService:add_town(session)
   radiant.check.is_string(session.player_id)
   radiant.check.is_string(session.faction)
   radiant.check.is_string(session.kingdom)

   assert(not self._sv.towns[session.player_id])

   local town = Town(session, radiant.create_datastore())
   self._sv.towns[session.player_id] = town
   self.__saved_variables:mark_changed()
   return town
end

function TownService:get_town(arg1)
   local player_id
   if type(arg1) == 'string' then
      player_id = arg1
   elseif radiant.util.is_a(arg1, Entity) then
      local unit_info = arg1:get_component('unit_info')
      if unit_info then
         player_id = unit_info:get_player_id()
      end
   end
   if not player_id then
      return nil
   end
   radiant.check.is_string(player_id)   
   return self._sv.towns[player_id]
end

return TownService

