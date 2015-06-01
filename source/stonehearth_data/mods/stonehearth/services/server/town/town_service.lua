local Entity = _radiant.om.Entity

local Town = require 'services.server.town.town'

local TownService = class()

function TownService:initialize()
   self._sv = self.__saved_variables:get_data() 

   if not self._sv.initialized then
      self._sv.towns = {}
      self._sv.initialized = true
   end
end

function TownService:add_town(player_id)
   radiant.check.is_string(player_id)

   assert(not self._sv.towns[player_id])

   local town = radiant.create_controller('stonehearth:town', player_id)
   self._sv.towns[player_id] = town
   self.__saved_variables:mark_changed()
   return town
end

--Called by the loot command on items dropped by enemies
function TownService:loot_item_command(session, response, item)
   local town = stonehearth.town:get_town(session.player_id)
   town:loot_item(item)
   return true
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

