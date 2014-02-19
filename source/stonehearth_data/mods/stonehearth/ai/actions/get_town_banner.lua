local Entity = _radiant.om.Entity
local GetTownBanner = class()

GetTownBanner.name = 'get town banner for entity'
GetTownBanner.does = 'stonehearth:get_town_banner'
GetTownBanner.args = {
   
}
GetTownBanner.think_output = {
   banner = Entity,      -- the town banner for the entity
}
GetTownBanner.version = 2
GetTownBanner.priority = 1

function GetTownBanner:start_thinking(ai, entity, args)
   local faction = radiant.entities.get_faction(entity)
   local town = stonehearth.town:get_town(faction)
   local banner = town:get_banner()

   ai:set_think_output({ banner = banner })
end

return GetTownBanner
