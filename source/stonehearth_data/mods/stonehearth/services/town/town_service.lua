local Town = require 'services.town.town'

local TownService = class()
function TownService:__init()
   self._towns = {}
end

function TownService:get_town(faction)
   -- todo: break out by faction, too!
   local town = self._towns[faction]
   if not town then
      town = Town(faction)
      self._towns[faction] = town
   end
   return town
end

return TownService()
