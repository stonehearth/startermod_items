local Entity = _radiant.om.Entity
local Town = require 'services.server.town.town'
local GetTownBanner = class()

GetTownBanner.name = 'get town banner for entity'
GetTownBanner.does = 'stonehearth:get_town_banner'
GetTownBanner.args = {
   town = Town
}
GetTownBanner.think_output = {
   banner = Entity,      -- the town banner for the entity
}
GetTownBanner.version = 2
GetTownBanner.priority = 1

function GetTownBanner:start_thinking(ai, entity, args)
   local banner = args.town:get_banner()
   if banner then
      ai:set_think_output({ banner = banner })
   end
   return
end

return GetTownBanner
