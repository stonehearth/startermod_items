local Entity = _radiant.om.Entity
local SitOnChair = class()

SitOnChair.name = 'sit on chair'
SitOnChair.does = 'stonehearth:sit_on_chair'
SitOnChair.args = { }
SitOnChair.version = 2
SitOnChair.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(SitOnChair)
         :execute('stonehearth:goto_item_made_of', { material = 'chair' })
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.item })
         :execute('stonehearth:sit_on_chair_adjacent', { chair = ai.BACK(2).item })

