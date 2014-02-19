local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local GotoTownBanner = class()

GotoTownBanner.name = 'goto town banner'
GotoTownBanner.does = 'stonehearth:goto_town_banner'
GotoTownBanner.args = {
   
}

GotoTownBanner.version = 2
GotoTownBanner.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GotoTownBanner)
         :execute('stonehearth:get_town_banner')
         :execute('stonehearth:goto_entity', {
            entity = ai.PREV.banner
         })
