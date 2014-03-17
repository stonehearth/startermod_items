local Entity = _radiant.om.Entity

local Town = require 'services.town.town'

local TownService = class()
function TownService:__init()
   self._towns = {}
end

function TownService:initialize()
end

function TownService:restore(saved_variables)
   radiant.log.write('town', 0, 'restore not implemented for town service!')
end

function TownService:add_town(session)
   radiant.check.is_string(session.player_id)
   radiant.check.is_string(session.faction)
   radiant.check.is_string(session.kingdom)

   assert(not self._towns[session.player_id])

   local town = Town(session)
   self._towns[session.player_id] = town
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
   assert(self._towns[player_id])
   return self._towns[player_id]
end

return TownService

